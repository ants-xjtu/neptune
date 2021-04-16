#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "PrivateHeap.h"

int main(int argc, char const *argv[])
{
#define HEAP_SIZE (1 << 24)
    void *heap = malloc(sizeof(uint8_t) * HEAP_SIZE);
    printf("[mb] allocate heap start at: %p\n", heap);
    SetHeap(heap, HEAP_SIZE, 0);
    InitHeap();

    void *mem100 = HeapMalloc(100);
    printf("[mb] HeapMalloc(100) from heap returns: %p\n", mem100);
    HeapFree(mem100);

    return 0;
}