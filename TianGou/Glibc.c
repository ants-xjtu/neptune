#include "TianGou.h"

// malloc
void *malloc(size_t size)
{
    return (interface.malloc)(size);
}

void free(void *object)
{
    (interface.free)(object);
}

void *realloc(void *object, size_t size)
{
    return (interface.realloc)(object, size);
}

void *calloc(size_t size, size_t count)
{
    return (interface.calloc)(size, count);
}

// crucial part of glibc
void exit(int stat)
{
    MESSAGE("nf recovering is not implemented");
    abort();
}

sighandler_t signal(int signum, sighandler_t handler)
{
    MESSAGE("nf try to set handler for signal %d", signum);
    if (signum != SIGINT && signum != SIGTERM)
    {
        return (interface.signal)(signum, handler);
    }
    return NULL;
}

char *strdup(const char *str)
{
    // toy version of strlen, and duplicate string on private heap
    const char *curr = str;
    size_t cnt = 0;
    while (*curr != '\0')
    {
        cnt++;
        curr++;
    }
    char *x = (char *)(interface.malloc)(cnt + 1);
    curr = str;
    char *cx = x;
    while (*curr != '\0')
    {
        *cx = *curr;
        cx++;
        curr++;
    }
    *cx = '\0';
    return x;
}