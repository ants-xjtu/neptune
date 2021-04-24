#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

namespace
{
    struct SwordHolderPass : public FunctionPass
    {
        static char ID;
        SwordHolderPass() : FunctionPass(ID) {}

        virtual bool runOnFunction(Function &F) override
        {
            if (F.getName() == "SwordHolder_CheckWriteMemory")
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
                        errs() << *op << "\n";
                        Value *pointer = op->getPointerOperand();
                        errs() << *pointer << "\n";
                        if (dyn_cast<AllocaInst>(pointer))
                        {
                            errs() << "...ignore alloca store\n";
                            continue;
                        }

                        IRBuilder<> builder(op);
                        Value *casted = builder.CreatePtrToInt(pointer, Type::getInt64Ty(Ctx));
                        Value *args[] = {casted};
                        builder.CreateCall(checkFunc, args);
                        modified = true;
                    }
                }
            }

            errs() << "After:\n"
                   << F;

            return modified;
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
