#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char *argv[]) {
    printf(
        "TODO:\n"
        "1. preload interface of loader (by YH)\n"
        "2. copy loader into neptune and bridge interfaces, wrap it into PrivateLoad\n"
        "3. create libtiangou.so, implement malloc APIs with PrivateHeap in it\n"
        "4. create demo executable for runtime, use PrivateLoad to load this MOON with libtiangou.so preloaded\n"
        "5. verify the following malloc is hijacked\n"
    );
    void *_p = malloc(sizeof(unsigned char) * 64);
    return 0;
}