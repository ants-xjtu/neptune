#ifndef PRIVATE_MALLOC_H
#define PRIVATE_MALLOC_H

#include <stddef.h>

void SetHeap(void *heap, size_t size);

void InitHeap(void);

void *HeapMalloc(size_t size);

void HeapFree(void *);

void *HeapRealloc(void *, size_t);

void *HeapCalloc(size_t, size_t);

#endif