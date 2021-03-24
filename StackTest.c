#include <stdio.h>
#include <stdlib.h>

#include "PrivateStack.h"

int count = 0;

void start() {
    if (count == 0) {
        printf("[private] initialize run\n");
        int x;
        printf("[private] stack variable at %p\n", &x);
    }
    while (1) {
        printf("[private] call virtual io (StackSwitch)\n");
        StackSwitch(-1);
        printf("[private] process packet #%d\n", count);
        count += 1;
    }
}

int main(int argc, const char *argv[]) {
    void *s = malloc(sizeof(unsigned char) * (4 << 10));
    printf("[runtime] created 4K stack at %p\n", s);
    printf("[runtime] initialize moon stack run\n");
    SetStack(0, s);
    StackStart(0, start);
    printf("[runtime] initialize finished\n");
    while (count < 4) {
        printf("process packet #%d\n", count);
        StackSwitch(0);
    }
    return 0;
}
