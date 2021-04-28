#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uintptr_t SwordHolder_MainPrefix;
#define SwordHolder_MainAlign (32ul << 30ul)
uintptr_t SwordHolder_ExtraLow, SwordHolder_ExtraHigh;

void SwordHolder_CheckWriteMemory(uintptr_t p)
{
    if ((p ^ SwordHolder_MainPrefix) < SwordHolder_MainAlign) {
        return;
    }
    if (p >= SwordHolder_ExtraLow && p < SwordHolder_ExtraHigh)
    {
        return;
    }
    fprintf(stderr, "[SwordHolder] invalid pointer writing at %p\n", (void *)p);
    abort();
}