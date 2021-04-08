#include <stdio.h>

#include "TianGou.h"

Interface interface;

void *malloc(size_t size)
{
    printf("[tiangou] calling malloc at %p, size = %lu\n", interface.malloc, size);
    void *object = (interface.malloc)(size);
    printf("[tiangou] malloc return = %p\n", object);
    return object;
}

void free(void *object)
{
    (interface.free)(object);
}