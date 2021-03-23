#include "Gmem.h"

namespace {
class Protection {
   protected:
    Module *M;
    InlineFunctionInfo *inliningInfo;
    Function *checkFunc;  // 具体sfi函数，其参数为一个指针

    Value *verifyPtr(Value *ptrVal, Instruction *I) {
        // 常量不处理
        if (isa<Constant>(ptrVal)) {
            To_Log("+ Ignoring constant " << *I);
            return ptrVal;
        }
        To_Log("Masking " << *I);
        // 在I前插入语句块
        IRBuilder<> B(I);
        // 把ptrVal转换成checkFunc的第一个参数的类型
        Value *funcArg = B.CreateBitCast(ptrVal, checkFunc->getFunctionType()->getParamType(0));
        // 即 masked = checkFunc(funcArg)
        Value *masked = B.CreateCall(checkFunc, {funcArg});
        // 把masked转换成ptrVal的值
        Value *casted = B.CreateBitCast(masked, ptrVal->getType());

        return casted;
    }

   public:
    // checkFunc从SFI module中找_Gmem_
    Module *SFI;
    Protection(Module *M) {
        this->M = M;
        this->inliningInfo = new InlineFunctionInfo();
        checkFunc = getcheckFunc(SFI, CheckMethod);
    }
    ~Protection() {}
    // 下面四个函数将访问地址的相应指针通过调用verifyPtr加上mask
    // 加上mask的指针就能够访问sensitive region了
    void handleLoadInst(LoadInst *LI) {
        if (LI->getMetadata(GmemSafeMDName) == NULL)
            LI->setOperand(0, verifyPtr(LI->getOperand(0), LI));
    }

    void handleStoreInst(StoreInst *SI) {
        if (SI->getMetadata(GmemSafeMDName) == NULL)
            SI->setOperand(1, verifyPtr(SI->getOperand(1), SI));
    }

    void handleLoadIntrinsic(MemTransferInst *MTI) {
        if (MTI->getMetadata(GmemSafeMDName) == NULL)
            MTI->setSource(verifyPtr(MTI->getRawSource(), MTI));
    }

    void handleStoreIntrinsic(MemIntrinsic *MI) {
        if (MI->getMetadata(GmemSafeMDName) == NULL)
            MI->setDest(verifyPtr(MI->getRawDest(), MI));
    }

    /* Verify pointer args to external functions if flag is set. */
    // 处理call/invoke函数
    void handleCallInst(CallSite &CS) {
        Function *F = CS.getCalledFunction();
        // 用户指定不处理
        if (!VerifyExternalCallArguments)
            return;
        // 调用了白名单中的函数
        if (callsIntoWhitelistedFunction(CS))
            return;
        // 不处理内联asm, 光从IR层面没办法解决
        if (CS.isInlineAsm())
            return;
        // 简洁调用
        if (!F)
            return; /* Indirect call */
        // 不处理内部函数
        if (!F->isDeclaration() && !F->isDeclarationForLinker())
            return; /* Not external */

        // 是intrinsic函数而且函数参数有指针，则需要判断
        // 只处理memcpy,memmove,memset,vastart,vacopy,vaend
        if (F->isIntrinsic() && hasPointerArg(F)) {
            switch (F->getIntrinsicID()) {
                case Intrinsic::dbg_declare:
                case Intrinsic::dbg_value:
                case Intrinsic::lifetime_start:
                case Intrinsic::lifetime_end:
                case Intrinsic::invariant_start:
                case Intrinsic::invariant_end:
                case Intrinsic::eh_typeid_for:
                case Intrinsic::eh_return_i32:
                case Intrinsic::eh_return_i64:
                case Intrinsic::eh_sjlj_functioncontext:
                case Intrinsic::eh_sjlj_setjmp:
                case Intrinsic::eh_sjlj_longjmp:
                    return; /* No masking */
                case Intrinsic::memcpy:
                case Intrinsic::memmove:
                case Intrinsic::memset:
                case Intrinsic::vastart:
                case Intrinsic::vacopy:
                case Intrinsic::vaend:
                    break; /* Continue with masking */
                default:
                    To_Log("Unhandled intrinsic that takes pointer: " << *F);
                    break; /* Do mask to be sure. */
            }
        }

        // 对所有参数中的指针执行verifyPtr进行mask
        Instruction *I = CS.getInstruction();
        for (unsigned i = 0, n = CS.getNumArgOperands(); i < n; i++) {
            Value *Arg = CS.getArgOperand(i);
            if (Arg->getType()->isPointerTy()) {
                Value *MaskedArg = verifyPtr(Arg, I);
                CS.getInstruction()->setOperand(i, MaskedArg);
            }
        }
    }

    /*
   * Inline calls to _Gmem_*. This is done afterwards,
   * instead of immediately, so the optimizeBB function can more easily
   * see (and optimize) region changes.
   */
    //只做一层Inline，一直循环到没有为止
    void inlineHelperCalls(Function &F) {
        bool has_changed;
        do {
            has_changed = false;
            for (inst_iterator it = inst_begin(F), E = inst_end(F); it != E; ++it) {
                Instruction *I = &(*it);
                CallInst *CI = dyn_cast<CallInst>(I);
                // 不是call指令
                if (!CI)
                    continue;
                Function *F = CI->getCalledFunction();
                if (!F)
                    continue;
                // 是gmem插入的指令
                if (F->getName().startswith("_Gmem_")) {
                    InlineFunction(CI, *inliningInfo);
                    has_changed = true;
                    break;
                }
            }
        } while (has_changed);
    }
};

struct GmemPass : public ModulePass {
   public:
    static char ID;

    GmemPass() : ModulePass(ID) {
        LLVMContext context;
        SMDiagnostic error;
        // prot->SFI = parseIRFile("./obj/Auxiliary.bc", error, context).release();
    }
    virtual bool runOnModule(Module &M);

   private:
    Protection *prot;

    void handleInst(Instruction *I) {
        if (LoadInst *LI = dyn_cast<LoadInst>(I)) {
            if (ReadWrite != WRITE)
                prot->handleLoadInst(LI);
        } else if (StoreInst *SI = dyn_cast<StoreInst>(I)) {
            if (ReadWrite != READ)
                prot->handleStoreInst(SI);
        } else if (MemIntrinsic *MI = dyn_cast<MemIntrinsic>(I)) {
            MemTransferInst *MTI = dyn_cast<MemTransferInst>(MI);
            if (MTI && ReadWrite != WRITE)
                prot->handleLoadIntrinsic(MTI);
            if (ReadWrite != READ)
                prot->handleStoreIntrinsic(MI);
        } else if (isa<CallInst>(I) || isa<InvokeInst>(I)) {
            CallSite CS(I);
            prot->handleCallInst(CS);
        }
    }
};

// 重载的runOnModule方法
bool GmemPass::runOnModule(Module &M) {
    To_Log("SFI started");

    // Get right instrumentation class (-based or domain-based)
    // this->prot = new Protection(&M);

    // // 遍历module中function
    // for (Function &F : M) {
    //     // 在白名单则跳过
    //     if (NO_Work_Needed(F, &WhitelistSection))
    //         continue;
    //     To_Log("Instrumenting " << F.getName());
    //     // 正式处理
    //     for (inst_iterator II = inst_begin(&F), E = inst_end(&F); II != E; ++II)
    //         handleInst(&*II);
    // }

    // Optimize inserted instrumentation further if needed.
    To_Log("Optimizing...");
    // 将应该处理的函数中的_Gmem_*全部inline
    // for (Function &F : M) {
    //     if (NO_Work_Needed(F, &WhitelistSection))
    //         continue;
    //     this->prot->inlineHelperCalls(F);
    // }

    return true;
}
}  // namespace
char GmemPass::ID = 0;
// 注册
static RegisterPass<GmemPass> X("gmem", "Gmem pass", true, false);

static void loadPass(const PassManagerBuilder &Builder, legacy::PassManagerBase &PM) {
    PM.add(new GmemPass());
}
// These constructors add our pass to a list of global extensions.
static RegisterStandardPasses GmemLoader_Ox(PassManagerBuilder::EP_OptimizerLast, loadPass);
// If the pass is enabled at any other points other than EP_EarlyAsPossible, we have to use EP_EnabledOnOptLevel0
static RegisterStandardPasses GmemLoader_O0(PassManagerBuilder::EP_EnabledOnOptLevel0, loadPass);