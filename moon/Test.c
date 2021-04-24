#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char *argv[])
{
    unsigned char *p = malloc(sizeof(unsigned char) * 64);
    printf("[Moon_Test] p = %p\n", p);
    for (int i = 0; i < 64; i += 1)
    {
        p[i] = i;
    }

    return 0;
}