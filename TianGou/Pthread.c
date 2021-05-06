#include "TianGou.h"

void *__tls_get_addr(void **p)
{
    MESSAGE("p = %p, *p = %p", p, *p);
    return *p;
}

int pthread_create(
    pthread_t *restrict thread,
    const pthread_attr_t *restrict attr,
    void *(*start_routine)(void *),
    void *restrict arg)
{
    MESSAGE("not implemented");
    abort();
}