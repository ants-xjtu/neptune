//
#ifndef PRIVATE_STACK_H
#define PRIVATE_STACK_H

#define MAX_STACK_NUM 4096

typedef void (*PrivateStart)(void);

// set start address of a private stack
void SetStack(int stackId, void *stack, size_t len);

// run `start` on an empty private stack until `StackSwitch` is called
void StackStart(int stackId, PrivateStart start);

// switch to a non-empty stack, e.g. running `start` and getting interrupted
// because `StackSwitch` is called
// return that calling
// so save current stack state, then resume another stack
// -1 means main (runtime) stack
void StackSwitch(int stackId);

// to setup a NF at stack#0:
// (runtime) SetStack(0, 0x4242), StackStart(0, start)
// (nf) virtual IO -> StackSwitch(-1)
// to process a packet along NFs at stack#0, #1 and #2
// (runtime) StackSwitch(0)
// (nf@stack#0) StackSwitch(1)
// (nf@stack#1) StackSwitch(2)
// (nf@stack#2) StackSwitch(-1)

// when we are jumping between pthreads, the TLS that record
// current stack ID will be lost
// call this to save/recover it, so the following StackSwitch calling
// will work even with stacks of different threads
void CrossThreadSaveStack();
void CrossThreadRestoreStack();

void DumpStack(const char *path, unsigned stackIdx);
void LoadStack(const char *path, unsigned stackIdx, FILE *reg);

#endif