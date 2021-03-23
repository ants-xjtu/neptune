#include "PrivateStack.h"

#define _GNU_SOURCE
#include <setjmp.h>

struct StackHeader {
    sigjmp_buf buf[2];
    int started;
    unsigned char tail;
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
        siglongjmp(globalStack->buf[1], 1);
    }
    if (sigsetjmp(globalStack->buf[0], 0) == 0) {
        globalStack->started = 1;
        // todo: deploy `start` on `&globalStack->tail`
        /* the first time an NF is called */
        start();
        return 0;
    } else {
        return 1;
    }
}

void StackEscape() {
    if (sigsetjmp(globalStack->buf[1], 0) == 0) {
        siglongjmp(globalStack->buf[0], 1);
    }
}
