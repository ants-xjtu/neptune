#include "AuxRunTime.h"

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

void *_Gmem_AndOr(void *ptr)
{
    return (void *)(((uintptr_t)ptr & _Gmem_SFI_MASK) | _Gmem_SFI_START);
}

void *_Gmem_IfElseHeap(void *ptr)
{
    uintptr_t p = (uintptr_t)ptr;
    if (unlikely((p >= _Gmem_Heap_Upper_Bound || p < _Gmem_Heap_Lower_Bound)))
    {
        fprintf(stderr, "Pointer %p illegal access(Not in Heap)\n", ptr);
        fprintf(stderr, "Segmentation Fault\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        return ptr;
    }
}

void *_Gmem_IfElseAll(void *ptr)
{
    uintptr_t p = (uintptr_t)ptr;
    // if (p < _Gmem_Heap_Upper_Bound && p >= _Gmem_Heap_Lower_Bound)
    //     return ptr;
    // else if (p < _Gmem_Stack_Upper_Bound && p >= _Gmem_Stack_Lower_Bound)
    //     return ptr;
    // //quick fix from Qcloud1223
    // else
    // {
    //     fprintf(stderr, "Pointer %p illegal access(Not in 2 protection fields)\n", ptr);
    //     fprintf(stderr, "Segmentation Fault\n");
    //     exit(EXIT_FAILURE);
    // }
    if (unlikely(p < _Gmem_Heap_Lower_Bound))
    {
        fprintf(stderr, "Pointer %p illegal access(Not in 2 protection fields)\n", ptr);
        fprintf(stderr, "Segmentation Fault\n");
        exit(EXIT_FAILURE);
    }
    if (unlikely(p >= _Gmem_Stack_Upper_Bound))
    {
        fprintf(stderr, "Pointer %p illegal access(Not in 2 protection fields)\n", ptr);
        fprintf(stderr, "Segmentation Fault\n");
        exit(EXIT_FAILURE);
    }
    if (p >= _Gmem_Heap_Upper_Bound)
    {
        if (unlikely(p < _Gmem_Stack_Lower_Bound))
        {
            fprintf(stderr, "Pointer %p illegal access(Not in 2 protection fields)\n", ptr);
            fprintf(stderr, "Segmentation Fault\n");
            exit(EXIT_FAILURE);
        }
        return ptr;
    }
    return ptr;
}

/* static size_t Gmem_pageround(size_t sz)
{
    int pgz = getpagesize();
    return (sz & ~(pgz - 1)) + pgz;
}*/

/* void *Gmem_alloc(size_t sz, void *start_addr)
{
    if (!last_alloc)
    {
        last_alloc = start_addr;
        SFI_START = Heap_Lower_Bound = (uintptr_t)start_addr;
    }
    fprintf(stderr, "Allocate at %p of size %zu, start %p\n", last_alloc, sz, start_addr);

    sz = _Gmem_pageround(sz);
    if ((uintptr_t)last_alloc + sz >= 1ULL << 47)
    {
        fprintf(stderr, "ERROR: cannot allocate %zu bytes at %p, would exceed address-space limit.\n", sz, last_alloc);
        exit(EXIT_FAILURE);
    }
    // Note: will trample over anything that may already be there.
    char *pg = mmap(last_alloc, sz, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
    if (pg == MAP_FAILED)
    {
        perror("mmap");
        return NULL;
    }
    last_alloc += sz;
    Heap_Upper_Bound = (uintptr_t)last_alloc;
    SFI_MASK = ((uintptr_t)last_alloc - SFI_START)-1;
    return pg;
}*/