#include <stdio.h>
#include <stdlib.h>

unsigned char g = '\0';
__thread int x;

int main(int argc, const char *argv[])
{
    unsigned char *h = malloc(sizeof(unsigned char) * 64);
    unsigned char s;
    // x = 42;
    printf("[Moon_Test] h = %p, &s = %p, &g = %p\n", h, &s, &g);

    unsigned char *choices[3] = {&s, h, &g};

    printf("Pick a base (0 = stack, 1 = heap, 2 = data): ");
    int i;
    scanf("%d", &i);
    printf("Input an offset: ");
    int off;
    scanf("%x", &off);

    printf("Attempt to write %p + (%#x)...", choices[i], off);
    choices[i][off] = 42;
    printf(" done\n");

    return 0;
}