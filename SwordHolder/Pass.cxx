#include <unordered_map>

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"

using namespace std;
using namespace llvm;

namespace
{
    const char *REASON_ALLOCA = "alloca";
    const char *REASON_GEP_WITH_CONST_INDICES = "gep w/ const indices";
    struct Stat
    {
        int storeInstCount = 0;
        int constantOperandCount = 0;
        unordered_map<string, int> protectedOperandMap, ignoredMap;

        Stat()
        {
            ignoredMap[REASON_ALLOCA] = 0;
            ignoredMap[REASON_GEP_WITH_CONST_INDICES] = 0;
        }

        void onStoreInst()
        {
            storeInstCount += 1;
        }
        void onIgnoredStoreInst(const char *reason)
        {
            ignoredMap[reason] += 1;
        }
        void onInsertCall(Value *operand)
        {
            string name;
            if (isa<Argument>(operand))
            {
                name = "<argument>";
            }
            else if (isa<GlobalVariable>(operand))
            {
                name = "<global>";
            }
            else if (auto *inst = dyn_cast<Instruction>(operand))
            {
                name = inst->getOpcodeName() + string("<inst>");
            }
            else if (auto *expr = dyn_cast<ConstantExpr>(operand))
            {
                name = expr->getOpcodeName() + string("<expr>");
            }
            else
            {
                errs() << "unknown value: " << *operand << "\n";
                return;
            }
            int count = protectedOperandMap.count(name) == 0 ? 0 : protectedOperandMap[name];
            protectedOperandMap[name] = count + 1;
        }

        void print()
        {
            outs() << "Total StoreInst: " << storeInstCount << "\n";
            outs() << "Ignored StoreInst\n";
            for (auto const &item : ignoredMap)
            {
                outs() << "  " << item.first << ": " << item.second << "\n";
            }
            outs() << "Protected StoreInst operand\n";
            for (auto const &item : protectedOperandMap)
            {
                outs() << "  " << item.first << ": " << item.second << "\n";
            }
        }
    };

    bool isSafeStore(StoreInst *op, Stat &stat)
    {
        Value *pointer = op->getPointerOperand();
        if (isa<AllocaInst>(pointer))
        {
            stat.onIgnoredStoreInst(REASON_ALLOCA);
            return true;
        }
        if (auto *gep = dyn_cast<GetElementPtrInst>(pointer))
        {
            if (gep->hasAllConstantIndices())
            {
                stat.onIgnoredStoreInst(REASON_GEP_WITH_CONST_INDICES);
                return true;
            }
        }
        if (auto *gepExpr = dyn_cast<GEPOperator>(pointer))
        {
            if (gepExpr->hasAllConstantIndices())
            {
                stat.onIgnoredStoreInst(REASON_GEP_WITH_CONST_INDICES);
                return true;
            }
        }
        // TODO
        if (auto *global = dyn_cast<GlobalVariable>(pointer))
        {
            if (global->hasExternalLinkage())
            {
                outs() << "Skip external:\n  " << *global << "\nIn\n";
                outs() << *op << "\n";
            }
            return true;
        }
        return false;
    }

    struct SwordHolderPass : public ModulePass
    {
        static char ID;
        SwordHolderPass() : ModulePass(ID) {}

        virtual bool runOnModule(Module &M) override
        {
            outs() << "[SwordHolder] Pass start: " << M.getName() << "\n";

            Stat stat;
            for (auto &F : M)
            {
                if (F.getName().startswith("SwordHolder_"))
                {
                    continue;
                }

                LLVMContext &Ctx = F.getContext();
                FunctionCallee checkFunc = F.getParent()->getOrInsertFunction(
                    "SwordHolder_CheckWriteMemory", Type::getVoidTy(Ctx), Type::getInt64Ty(Ctx));

                vector<CallInst *> checkCalls;
                for (auto &B : F)
                {
                    for (auto &I : B)
                    {
                        if (I.mayWriteToMemory())
                        {
                            if (isa<CallInst>(I) || isa<InvokeInst>(I) || isa<LoadInst>(I))
                            {
                                continue;
                            }
                            auto *op = dyn_cast<StoreInst>(&I);
                            if (!op)
                            {
                                errs() << "unexpected: " << I << "\n";
                                continue;
                            }
                            stat.onStoreInst();
                            if (isSafeStore(op, stat))
                            {
                                continue;
                            }

                            Value *operand = op->getPointerOperand();
                            stat.onInsertCall(operand);
                            IRBuilder<> builder(op);
                            Value *casted = builder.CreatePtrToInt(operand, Type::getInt64Ty(Ctx));
                            Value *args[] = {casted};
                            CallInst *call = builder.CreateCall(checkFunc, args);
                            checkCalls.push_back(call);
                        }
                    }
                }
                for (CallInst *call : checkCalls)
                {
                    InlineFunctionInfo ifi;
                    InlineFunction(call, ifi);
                }
            }
            outs() << "[SwordHolder] Pass summary\n";
            stat.print();
            // M.print(outs(), nullptr);
            return true;
        }
    };
}

char SwordHolderPass::ID = 0;
static RegisterPass<SwordHolderPass> X("SwordHolder", "Experimental SwordHolder Pass for HyperMOON", false, false);

static void loadPass(const PassManagerBuilder &Builder, legacy::PassManagerBase &PM)
{
    PM.add(new SwordHolderPass());
}
static RegisterStandardPasses Y_Ox(PassManagerBuilder::EP_ModuleOptimizerEarly, loadPass);
