#ifndef PRIVATE_MALLOC_H
#define PRIVATE_MALLOC_H

#include <stddef.h>

#define MAX_MOON_NUM 4096
void *HeapStart[MAX_MOON_NUM], *HeapEnd[MAX_MOON_NUM];

void SetHeap(void *heap, size_t size, int moonId);

void HeapSwitch(int moonId);

void InitHeap(void);

void *HeapMalloc(size_t size);

void HeapFree(void *);

void *HeapRealloc(void *, size_t);

void *HeapAlignedAlloc(size_t align, size_t size);

void *HeapCalloc(size_t, size_t);

void *HeapInfo();

#endif