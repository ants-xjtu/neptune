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
    struct Stat
    {
        int storeInstCount = 0, allocaOperandCount = 0, constantOperandCount = 0;
        unordered_map<const char *, int> operandCountMap;
        bool printedModuleName = false;

        void onStoreInst()
        {
            storeInstCount += 1;
        }
        void onAllocaOperand()
        {
            allocaOperandCount += 1;
        }
        void onConstant()
        {
            constantOperandCount += 1;
        }
        void onInsertCall(Value *operand)
        {
            auto *inst = dyn_cast<Instruction>(operand);
            const char *name = !inst ? "function argument" : inst->getOpcodeName();
            int count = operandCountMap.count(name) == 0 ? 0 : operandCountMap[name];
            operandCountMap[name] = count + 1;
        }

        void print()
        {
            outs() << "Total StoreInst: " << storeInstCount << "\n";
            outs() << "Ignored to-Alloca storing: " << allocaOperandCount << "\n";
            outs() << "Ignored to-Constant storing: " << constantOperandCount << "\n";
            outs() << "Kinds of StoreInst operand\n";
            for (auto const &item : operandCountMap)
            {
                outs() << "  " << item.first << ": " << item.second << "\n";
            }
        }
    };
    struct SwordHolderPass : public ModulePass
    {
        static char ID;
        SwordHolderPass() : ModulePass(ID)
        {
            outs() << "[SwordHolder] pass start\n";
        }

        virtual bool runOnModule(Module &M) override
        {
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
                        if (I.mayWriteToMemory() && !isa<CallInst>(I) && !isa<InvokeInst>(I))
                        {
                            auto *op = dyn_cast<StoreInst>(&I);
                            if (!op) {
                                outs() << "skipped: " << I << "\n";
                                continue;
                            }
                            stat.onStoreInst();
                            Value *pointer = op->getPointerOperand();
                            if (isa<AllocaInst>(pointer))
                            {
                                stat.onAllocaOperand();
                                continue;
                            }
                            else if (isa<Constant>(pointer))
                            {
                                stat.onConstant();
                                continue;
                            }

                            stat.onInsertCall(pointer);
                            IRBuilder<> builder(op);
                            Value *casted = builder.CreatePtrToInt(pointer, Type::getInt64Ty(Ctx));
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
            outs() << "[SwordHolder] pass stat:\n";
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
