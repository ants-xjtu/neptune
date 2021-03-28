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
static int currStackId;

void SetStack(int stackId, void *stack) {
    nfStack[stackId][0] = stack;
}

void SetStackPC(int stackId, void *pc) {
    nfStack[stackId][1] = pc;
}

// void InitStack() {
//     globalStack->started = 0;
// }

void StackSwitch(int stackId) {
    //I know this is ugly... I will fix it later
    int realStack = (stackId < 0) ? (MAX_STACK_NUM - 1) : stackId;
    int realCurrStack = (currStackId < 0) ? (MAX_STACK_NUM - 1) : currStackId;
    currStackId = stackId;
    StackSwitchAsm(&nfStack[realCurrStack], &nfStack[realStack]);
}

void StackStart(int stackId, PrivateStart start) {
    //note that this function will only be called once upon a SetStack'd stack
    //after the private stack is set up, trampoline and vio call symmetric `StackSwitch`
    void *stackTop = nfStack[stackId][0];
    currStackId = stackId;
    //this is a bootstrap problem: we need to store status on main stack before first
    //`stackswitch` is called
    //so though this looks ugly and asymmetric, we need to embrace the inevitable
    if (SaveState(&nfStack[MAX_STACK_NUM - 1]) == 0) {
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

// int StackRun(PrivateStart start) {
// if (globalStack->started) {
// /* the second time an NF is called */
// if (SaveState(globalStack->TrampoStack) == 0) {
// RestoreState(globalStack->MoonStack);
// } else {
// printf("return to runtime from private stack\n");
// }
// } else {
// globalStack->started = 1;
// if (SaveState(globalStack->TrampoStack) == 0) {
// TODO: correctly set the stack location here
// start();
// return 0;
// } else {
// return 1;
// }
// }
// }

// void StackEscape() {
// if (SaveState(globalStack->MoonStack) == 0) {
// RestoreState(globalStack->TrampoStack);
// } else {
// printf("jumping into private stack from trampoline, now returning to vio\n");
// }
// }
