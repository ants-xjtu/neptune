#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

extern struct
{
    uint64_t low, high;
} SwordHolder_Region;

__attribute__((always_inline)) void SwordHolder_CheckWriteMemory(uint64_t pointer)
{
    if (pointer < SwordHolder_Region.low)
    {
        goto error;
    }
    if (pointer >= SwordHolder_Region.high)
    {
        goto error;
    }
    return;
error:
    fprintf(stderr, "[SwordHolder] invalid pointer writing at %p\n", (void *)pointer);
    abort();
}