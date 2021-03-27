#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../PrivateStack.h"

int count = 0;

void start() {
    if (count == 0) {
        // printf("[private] initialize run\n");
        int x;
        // printf("[private] stack variable at %p\n", &x);
    }
    while (1) {
        // printf("[private] call virtual io (StackSwitch)\n");
        StackSwitch(-1);
        // printf("[private] process packet #%d\n", count);
        count += 1;
    }
}

int main(int argc, const char *argv[]) {
    int stackSize = sizeof(unsigned char) * (4 << 10);
    void *s = malloc(stackSize);
    int packetCount = 1000000;
    printf("[runtime] created 4K stack at %p\n", s);
    printf("[runtime] initialize moon stack run\n");
    // TODO: someone says this isn't aligned to anything, we shall see
    SetStack(0, s + stackSize - 0x10);
    StackStart(0, start);
    printf("[runtime] initialize finished\n");
    struct timespec ts = {0, 0}, te = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    while (count < packetCount) {
        // printf("[runtime] process packet #%d\n", count);
        StackSwitch(0);
    }
    clock_gettime(CLOCK_MONOTONIC, &te);
    double t = ((double)te.tv_sec + 1.0e-9 * te.tv_nsec) -
               ((double)ts.tv_sec + 1.0e-9 * ts.tv_nsec);
    printf("Time elapsed: %lf seconds per %d switch\n", t, packetCount * 2);
    return 0;
}
