#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

using namespace llvm;

namespace
{
    struct SwordHolderPass : public FunctionPass
    {
        static char ID;
        SwordHolderPass() : FunctionPass(ID) {}

        virtual bool runOnFunction(Function &F) override
        {
            errs() << "I saw a function called " << F.getName() << "!\n";
            return false;
        }
    };
}

char SwordHolderPass::ID = 0;
static RegisterPass<SwordHolderPass> X("SwordHolder", "Experimental SwordHolder Pass for HyperMOON", false, false);

static void loadPass(const PassManagerBuilder &Builder, legacy::PassManagerBase &PM)
{
    PM.add(new SwordHolderPass());
}
// These constructors add our pass to a list of global extensions.
static RegisterStandardPasses Y_Ox(PassManagerBuilder::EP_ModuleOptimizerEarly, loadPass);
// If the pass is enabled at any other points other than EP_EarlyAsPossible, we have to use EP_EnabledOnOptLevel0
static RegisterStandardPasses Y_O0(PassManagerBuilder::EP_EnabledOnOptLevel0, loadPass);