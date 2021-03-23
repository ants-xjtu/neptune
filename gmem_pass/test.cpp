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
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Pass.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/ErrorOr.h>
#include <llvm/Bitcode/BitcodeReader.h>

#include <string>
#include <iostream>
using namespace llvm;


int main()
{
    LLVMContext context;
    SMDiagnostic error;
    auto SFI = parseIRFile("Auxiliary.bc", error, context);

    std::cout << "Successfully read Module:" << std::endl;
    std::cout << " Name: " << SFI->getName().str() << std::endl;
    std::cout << " Target triple: " << SFI->getTargetTriple() << std::endl;
    std::cout << " FunctionList Size: " << SFI->getFunctionList().size() << std::endl;

    for (auto iter1 = SFI->getFunctionList().begin(); iter1 != SFI->getFunctionList().end(); iter1++)
    {
        Function &f = *iter1;
        std::cout << " Function: " << f.getName().str() << std::endl;
        for (auto iter2 = f.getBasicBlockList().begin(); iter2 != f.getBasicBlockList().end(); iter2++)
        {
            BasicBlock &bb = *iter2;
            std::cout << "  BasicBlock: " << bb.getName().str() << std::endl;
            for (auto iter3 = bb.begin(); iter3 != bb.end(); iter3++)
            {
                Instruction &inst = *iter3;
                std::cout << "   Instruction " << &inst << " : " << inst.getOpcodeName();
                unsigned int i = 0;
                unsigned int opnt_cnt = inst.getNumOperands();
                for (; i < opnt_cnt; ++i)
                {
                    Value *opnd = inst.getOperand(i);
                    std::string o;
                    //          raw_string_ostream os(o);
                    //         opnd->print(os);
                    //opnd->printAsOperand(os, true, m);
                    if (opnd->hasName())
                    {
                        o = opnd->getName();
                        std::cout << " " << o << ",";
                    }
                    else
                    {
                        std::cout << " ptr" << opnd << ",";
                    }
                }
                std::cout << std::endl;
            }
        }
    }
    return 0;
}