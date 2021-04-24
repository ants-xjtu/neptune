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

        virtual bool runOnFunction(Function &F)
        {
            outs() << "I saw a function called " << F.getName() << "!\n";
            return false;
        }
    };
}

char SwordHolderPass::ID = 0;

// Automatically enable the pass.
// http://adriansampson.net/blog/clangpass.html
static void registerSwordHolderPass(const PassManagerBuilder &,
                                    legacy::PassManagerBase &PM)
{
    PM.add(new SwordHolderPass());
}
static RegisterStandardPasses
    RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                   registerSwordHolderPass);