#include <unordered_map>

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace std;
using namespace llvm;

namespace
{
    struct Stat
    {
        int storeInstCount = 0, allocaOperandCount = 0, constantOperandCount = 0;
        unordered_map<const char *, int> operandCountMap;
        bool printedModuleName = false;

        void onFunction(Function &F)
        {
            if (!printedModuleName)
            {
                outs() << "module name: " << F.getParent()->getName() << "\n";
                printedModuleName = true;
            }
        }

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
            const char *name = !inst ? "non-instruction operand" : inst->getOpcodeName();
            int count = operandCountMap.count(name) == 0 ? 0 : operandCountMap[name];
            operandCountMap[name] = count + 1;
        }

        void print()
        {
            outs() << "Total StoreInst: " << storeInstCount << "\n";
            outs() << "Ignored to-Alloca storing: " << allocaOperandCount << "\n";
            outs() << "Dangerous to-Constant storing: " << constantOperandCount << "\n";
            outs() << "Kinds of StoreInst operand\n";
            for (auto const &item : operandCountMap)
            {
                outs() << "  " << item.first << ": " << item.second << "\n";
            }
        }
    };
    struct SwordHolderPass : public FunctionPass
    {
        static char ID;
        SwordHolderPass() : FunctionPass(ID)
        {
            outs() << "[SwordHolder] pass start\n";
        }

        Stat stat;

        virtual bool runOnFunction(Function &F) override
        {
            stat.onFunction(F);
            if (F.getName().startswith("SwordHolder_"))
            {
                return false;
            }

            bool modified = false;

            LLVMContext &Ctx = F.getContext();
            FunctionCallee checkFunc = F.getParent()->getOrInsertFunction(
                "SwordHolder_CheckWriteMemory", Type::getVoidTy(Ctx), Type::getInt64Ty(Ctx));

            for (auto &B : F)
            {
                for (auto &I : B)
                {
                    if (auto *op = dyn_cast<StoreInst>(&I))
                    {
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
                        }

                        stat.onInsertCall(pointer);
                        IRBuilder<> builder(op);
                        Value *casted = builder.CreateBitCast(pointer, Type::getVoidTy(Ctx)->getPointerTo());
                        Value *args[] = {casted};

                        builder.CreateCall(checkFunc, args);
                        modified = true;
                    }
                }
            }
            return modified;
        }

        ~SwordHolderPass()
        {
            stat.print();
        }
    };
}

char SwordHolderPass::ID = 0;
static RegisterPass<SwordHolderPass> X("SwordHolder", "Experimental SwordHolder Pass for HyperMOON", false, false);

static void loadPass(const PassManagerBuilder &Builder, legacy::PassManagerBase &PM)
{
    PM.add(new SwordHolderPass());
}
static RegisterStandardPasses Y_Ox(PassManagerBuilder::EP_EarlyAsPossible, loadPass);
