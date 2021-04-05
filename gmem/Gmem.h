#ifndef GMEM_PASS_H
#define GMEM_PASS_H

#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Support/CommandLine.h>

#include <string>

using namespace llvm;

#define To_Log(line) (errs() << "[Gmem] " << line << '\n')

enum readwrite
{
	READ,
	WRITE,
	READWRITE,
};

enum checkmethod
{
	AndOr,
	IfElseHeap,
	IfElseAll,
};

cl::opt<readwrite> ReadWrite(
	"Gmem-rw",
	cl::desc("What type of memory accesses to protect when doing sfi:"),
	cl::values(
		clEnumValN(READWRITE, "rw", "Reads and writes"),
		clEnumValN(READ, "r", "Reads only"),
		clEnumValN(WRITE, "w", "Writes only")),
	cl::init(WRITE));

cl::opt<checkmethod> CheckMethod(
	"Gmem-check-method",
	cl::desc("What type of ptr check to use when doing sfi:"),
	cl::values(
		clEnumValN(AndOr, "andor", "And Or Masking"),
		clEnumValN(IfElseHeap, "ifelseheap", "If Else Heap Boundary Check Only on Heap"),
		clEnumValN(IfElseAll, "ifelseall", "If Else Boundary Check on Stack and Heap")),
	cl::init(IfElseHeap));

cl::opt<bool> VerifyExternalCallArguments(
	"Gmem-verify-external-call-args",
	cl::desc("add checks to all pointer-type arguments to external functions"
			 "(make sure uninstrumented libraries cannot use invalid pointers.)"),
	cl::init(true));

// 指定白名单section
cl::opt<std::string> WhitelistSection(
	"Gmem-whitelist-section",
	cl::desc("Functions in this section are allowed access to the safe region"),
	cl::init("Gmem_functions"));

static const std::string GmemSafeMDName = "Gmem.allowedaccess";

void Gmem_saferegion_access(Instruction *I)
{
	I->setMetadata(GmemSafeMDName, MDNode::get(I->getContext(), {}));
}

// 在地址空间里找checkFunc
static Function *getcheckFunc(Module *M, enum checkmethod check_method)
{
	std::string func_name = "_Gmem_";
	switch (check_method)
	{
	case AndOr:
		func_name += "AndOr";
		break;
	case IfElseHeap:
		func_name += "IfElseHeap";
		break;
	case IfElseAll:
		func_name += "IfElseAll";
		break;
	default:
		To_Log("Not yet supported");
		break;
	}
	Function *F = M->getFunction(func_name);
	if (!F)
	{
		To_Log("Cannot find func for method" << func_name);
		exit(EXIT_FAILURE);
	}
	return F;
}

// 不必对F做处理
bool NO_Work_Needed(llvm::Function &F, std::string *ignoreSection = nullptr)
{
	if (F.isDeclaration() || F.getName().startswith("_Gmem_"))
		return true;
	if (ignoreSection && !std::string(F.getSection()).compare(*ignoreSection))
		return true;
	return false;
}

// 函数参数是否有指针类型
bool hasPointerArg(Function *F)
{
	FunctionType *FT = F->getFunctionType();
	for (unsigned i = 0, n = FT->getNumParams(); i < n; i++)
	{
		Type *type = FT->getParamType(i);
		if (type->isPointerTy())
			return true;
	}
	return false;
}

// 返回是否直接调用了白名单section中的函数
bool callsIntoWhitelistedFunction(CallSite &CS)
{
	Function *F = CS.getCalledFunction();
	if (!F) // Indirect call
		return false;
	return F->getSection() == WhitelistSection;
}
#endif /* GMEM_PASS_H */