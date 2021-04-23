#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

uintptr_t gmemLowRegionLow, gmemLowRegionHigh, gmemHighRegionLow, gmemHighRegionHigh;

void *_Gmem_IfElseAll(void *ptr)
{
    uintptr_t p = (uintptr_t)ptr;

    if (unlikely(p < gmemLowRegionLow))
    {
        goto abort;
    }
    if (unlikely(p >= gmemHighRegionHigh))
    {
        goto abort;
    }
    if (p >= gmemLowRegionHigh)
    {
        if (unlikely(p < gmemHighRegionLow))
        {
            goto abort;
        }
    }
    return ptr;
abort:
    fprintf(stderr, "Pointer %p illegal access(Not in two protection fields)\n", ptr);
    fprintf(stderr, "Segmentation Fault\n");
    exit(EXIT_FAILURE);
}

// add by Qcloud1223
// this is the ideal situation: with some analysis, every memory write has only one branch
// still borrow the historical name ifelseheap, and use low-low and low-high as prot region
void *_Gmem_IfElseHeap(void *ptr)
{
    uintptr_t p = (uintptr_t)ptr;
    if (unlikely(p < gmemLowRegionLow))
    {
        goto abort;
    }
    if (unlikely(p >= gmemLowRegionHigh))
    {
        goto abort;
    }
    return ptr;
abort:
    fprintf(stderr, "Pointer %p illegal access(Not in single protection fields)\n", ptr);
    fprintf(stderr, "Segmentation Fault\n");
    exit(EXIT_FAILURE);
}