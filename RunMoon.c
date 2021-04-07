#define _ISOC11_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "PrivateHeap.h"
#include "PrivateStack.h"
#include "PrivateLoad.h"

const size_t STACK_SIZE = 32ul << 20; // 32MB
const size_t MOON_SIZE = 32ul << 30;  // 32GB
// Heap size = MOON_SIZE - STACK_SIZE - library.length
const int MOON_ID = 0;
const char *DONE_STRING = "\xe2\x86\x91 done";

int main(int argc, char *argv[], char *envp[])
{
    if (argc < 2)
    {
        printf("usage: %s <moon>\n", argv[0]);
        return 0;
    }
    const char *moonPath = argv[1];
    InitLoader(argc, argv, envp);

    printf("allocating memory for MOON %s\n", moonPath);
    void *arena = aligned_alloc(sysconf(_SC_PAGESIZE), MOON_SIZE);
    printf("arena address %p size %#lx\n", arena, MOON_SIZE);

    struct PrivateLibrary library;
    library.file = moonPath;
    LoadLibrary(&library);
    printf("library requires space: %#lx\n", library.length);
    void *stackStart = arena, *libraryStart = arena + STACK_SIZE;
    void *heapStart = libraryStart + library.length;
    size_t heapSize = MOON_SIZE - STACK_SIZE - library.length;

    printf("*** MOON memory layout ***\n");
    printf("Stack\t%p - %p\tsize: %#lx\n", stackStart, stackStart + STACK_SIZE, STACK_SIZE);
    printf("Library\t%p - %p\tsize: %#lx\n", libraryStart, libraryStart + library.length, library.length);
    printf("Heap\t%p - %p\tsize: %#lx\n", heapStart, heapStart + heapSize, heapSize);
    printf("\n");

    printf("initializing regions\n");
    SetStack(MOON_ID, stackStart, STACK_SIZE);
    library.loadAddress = libraryStart;
    DeployLibrary(&library);
    SetHeap(heapStart, heapSize);
    InitHeap();
    printf("%s\n", DONE_STRING);

    return 0;
}