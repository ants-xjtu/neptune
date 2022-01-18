#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

static int savedCurrStackId;
static RegSet savedMainStack;
void CrossThreadSaveStack()
{
    savedCurrStackId = currStackId;
    memcpy(savedMainStack, mainStack, sizeof(RegSet));
}
void CrossThreadRestoreStack()
{
    currStackId = savedCurrStackId;
    memcpy(mainStack, savedMainStack, sizeof(RegSet));
}

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

void DumpStack(const char *path, unsigned stackIdx)
{
    FILE *reg = fopen(path, "wb");
    char regBuf[16];
    for (int i = 0; i < 9; i++)
    {
        sprintf(regBuf, "%p", nfStack[stackIdx][i]);
        // do not write the trailing '\0'
        fwrite(regBuf, strlen(regBuf), 1, reg);
        fwrite("\n", sizeof(char), 1, reg);
    }
    fclose(reg);
}

void LoadStack(const char *path, unsigned stackIdx)
{
    // fopen will segfault
    FILE *reg = fopen(path, "rb");
    char regBuf[16];
    void *r;
    for (int i = 0; i < 9; i++)
    {
        fgets(regBuf, 16, reg);
        sscanf(regBuf, "%p", &r);
        // when dumping, sprintf will try to convert NULL to (nil)
        nfStack[stackIdx][i] = r;
    }
    fclose(reg);
    // nfStack[stackIdx][0] = (void *)0x7fff59335d90;
    // nfStack[stackIdx][1] = (void *)0x55555555f5bc;
    // nfStack[stackIdx][2] = (void *)0x7fff59335db0;
    // nfStack[stackIdx][3] = NULL;
    // nfStack[stackIdx][4] = (void *)0x7fff59335ec0;
    // nfStack[stackIdx][5] = NULL;
    // nfStack[stackIdx][6] = NULL;
    // nfStack[stackIdx][7] = NULL;
    // nfStack[stackIdx][8] = (void *)0x1fa00000037f;
}
