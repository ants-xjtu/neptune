#include <stdio.h>

#include "TianGou.h"

Interface interface;

void *malloc(size_t size)
{
    printf("[tiangou] calling malloc\n");
    return (interface.malloc)(size);
}

void free(void *object)
{
    (interface.free)(object);
}