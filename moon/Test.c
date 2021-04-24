#include <stdio.h>
#include <stdlib.h>

extern int unknownValueSource[32];

int main(int argc, const char *argv[])
{
    unsigned char *p = malloc(sizeof(unsigned char) * 64);
    // printf("[Moon_Test] p = %p\n", p);
    for (int i = 0; i < 32; i += 1)
    {
        p[i + 16] = unknownValueSource[i];
    }
    printf("p[42] = %d\n", p[42]);

    return 0;
}