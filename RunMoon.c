#define _ISOC11_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "PrivateHeap.h"
#include "PrivateStack.h"
#include "PrivateLoad.h"
#include "TianGou.h"

const size_t STACK_SIZE = 32ul << 20; // 32MB
const size_t MOON_SIZE = 32ul << 30;  // 32GB
// Heap size = MOON_SIZE - STACK_SIZE - library.length
const int MOON_ID = 0;
const char *DONE_STRING = "\xe2\x86\x91 done";

// uintptr_t gmemLowRegionLow, gmemLowRegionHigh, gmemHighRegionLow, gmemHighRegionHigh;

int (*moonStart)(int argc, char *argv[]);
void initMoon();

int main(int argc, char *argv[], char *envp[])
{
    if (argc < 3)
    {
        printf("usage: %s <tiangou> <moon>\n", argv[0]);
        return 0;
    }
    const char *tiangouPath = argv[1], *moonPath = argv[2];
    InitLoader(argc, argv, envp);

    printf("loading tiangou library %s\n", tiangouPath);
    PreloadLibrary(tiangouPath);
    //

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

    printf("setting protection region for MOON\n");
    uintptr_t *gmemLowRegionLow = LibraryFind(&library, "gmemLowRegionLow");
    uintptr_t *gmemLowRegionHigh = LibraryFind(&library, "gmemLowRegionHigh");
    uintptr_t *gmemHighRegionLow = LibraryFind(&library, "gmemHighRegionLow");
    uintptr_t *gmemHighRegionHigh = LibraryFind(&library, "gmemHighRegionHigh");
    *gmemLowRegionLow = *gmemLowRegionHigh = 0;
    *gmemHighRegionLow = (uintptr_t)arena;
    *gmemHighRegionHigh = *gmemHighRegionLow + MOON_SIZE;
    printf("low region:\t%#lx - %#lx\n", *gmemLowRegionLow, *gmemLowRegionHigh);
    printf("high region:\t%#lx - %#lx\n", *gmemHighRegionLow, *gmemHighRegionHigh);

    moonStart = LibraryFind(&library, "main");
    printf("entering MOON for initial running, start = %p\n", moonStart);
    StackStart(MOON_ID, initMoon);

    return 0;
}

void initMoon()
{
    char *argv[0];
    printf("[initMoon] calling moonStart\n");
    moonStart(0, argv);
}