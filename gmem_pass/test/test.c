#include <stdio.h>
#include <stdlib.h>

#include "AuxRunTime.h"

__attribute__ ((section("safe_functions"), noinline))
char safe_region_read(volatile char *ptr) {
    return *ptr;
}
__attribute__ ((section("safe_functions"), noinline))
void safe_region_write(volatile char *ptr, char v) {
    *ptr = v;
}

int main(int argc, char **argv) {
    puts("start");
    volatile char *a = Gmem_alloc(10, (void *)0x400000000000ULL);
    volatile char *b = malloc(10);

    fprintf(stderr, "alloc: safe: %p  normal: %p\n", a, b);

    safe_region_write(a, 0x33);
    *b = 0x20; 
    char c = safe_region_read(a);
    fprintf(stderr, "Read %x from %p\n", c, a);

    fprintf(stderr, "The next line should not work (read from safe %p)\n", a);
    //char invalid_read = *a;
    //fprintf(stderr, "Read succeeded, value: %x (correct read: %x)\n", invalid_read, safe_region_read(a));
    *a = 0x44;
    fprintf(stderr, "Write succeeded, now a = %x\n", safe_region_read(a));
    return 0;
}