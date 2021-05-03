#include <stdint.h>
#include <stdio.h>

#include "PrivateStack.h"

typedef void *RegSet[9];
extern int SaveState(RegSet *regs);
extern void RestoreState(RegSet *regs);
extern void StackSwitchAsm(RegSet *src, RegSet *dst);
static RegSet nfStack[MAX_STACK_NUM];

// struct StackHeader {
// RegSet MoonStack;
// RegSet TrampoStack;
// int started;
// unsigned char tail;
// TODO: explain where the memory comes from...
// };

// static struct StackHeader *globalStack;
static __thread int currStackId = -1;
static __thread RegSet mainStack;

void SetStack(int stackId, void *stack, size_t len)
{
    void *StackBottom = stack + len;
    //bring it back to mapped region, and kill least significant 4 bits
    uint64_t StackBottomAligned = (((uint64_t)StackBottom - 1) >> 4) << 4;
    nfStack[stackId][0] = (void *)StackBottomAligned;
}

void StackSwitch(int stackId)
{
    // //I know this is ugly... I will fix it later
    // int realStack = (stackId < 0) ? (MAX_STACK_NUM - 1) : stackId;
    // int realCurrStack = (currStackId < 0) ? (MAX_STACK_NUM - 1) : currStackId;
    RegSet *current = currStackId < 0 ? &mainStack : &nfStack[currStackId];
    RegSet *next = stackId < 0 ? &mainStack : &nfStack[stackId];
    currStackId = stackId;
    // StackSwitchAsm(&nfStack[realCurrStack], &nfStack[realStack]);
    StackSwitchAsm(current, next);
}

void StackStart(int stackId, PrivateStart start)
{
    //note that this function will only be called once upon a SetStack'd stack
    //after the private stack is set up, trampoline and vio call symmetric `StackSwitch`
    void *stackTop = nfStack[stackId][0];
    currStackId = stackId;
    //this is a bootstrap problem: we need to store status on main stack before first
    //`stackswitch` is called
    //so though this looks ugly and asymmetric, we need to embrace the inevitable
    // if (SaveState(&nfStack[MAX_STACK_NUM - 1]) == 0)
    if (!SaveState(&mainStack))
    {
        //rebase rsp only once, for later %rsp is saved in RegSet
        asm(
            "movq %0, %%rsp "
            :
            : "rm"(stackTop)
            :);
        //wait `stackswitch` called in vio, and return here
        start();
    }
    //now that we've done initialize NF, we are now in main stack
    currStackId = -1;
}
