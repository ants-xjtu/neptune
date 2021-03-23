#include <stdio.h>
#include <stdlib.h>

#include "AuxRunTime.h"

int main(int argc, char **argv) {
    puts("start");
    volatile uintptr_t *a = Gmem_alloc(64, (void *)0x400000000000ULL);

    //safe_region_write(a, 0x33);
    *a = 5;

    char c = *a;
    fprintf(stderr, "Read %x from %p\n", c, a);

    fprintf(stderr, "The next line should work (write to safe %p)\n", a);
    for (size_t i = 0; i < 499999993 && (*a)%499999993 != 1; i++)
    {
        *a = ((*a)*5)%499999993;
    }

    fprintf(stderr, "Write succeeded, now a = %lu\n", *a);
    return 0;
}