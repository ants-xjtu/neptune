#include <stdint.h>
#include <stdio.h>

#include "PrivateStack.h"

typedef void *RegSet[9];
extern int SaveState(RegSet *regs);
extern void RestoreState(RegSet *regs);

struct StackHeader {
    RegSet MoonStack;
    RegSet TrampoStack;
    int started;
    unsigned char tail;
    //TODO: explain where the memory comes from...
};

static struct StackHeader *globalStack;

void SetStack(void *stack) {
    globalStack = stack;
}

void InitStack() {
    globalStack->started = 0;
}

int StackRun(PrivateStart start) {
    if (globalStack->started) {
        /* the second time an NF is called */
        if (SaveState(globalStack->TrampoStack) == 0) {
            RestoreState(globalStack->MoonStack);
        } else {
            printf("return to runtime from private stack\n");
        }
    } else {
        globalStack->started = 1;
        if (SaveState(globalStack->TrampoStack) == 0) {
            //TODO: correctly set the stack location here
            start();
            return 0;
        } else {
            return 1;
        }
    }
}

void StackEscape() {
    if (SaveState(globalStack->MoonStack) == 0) {
        RestoreState(globalStack->TrampoStack);
    } else {
        printf("jumping into private stack from trampoline, now returning to vio\n");
    }
}
