#include "TianGou.h"

void *__tls_get_addr(void **p)
{
    MESSAGE("p = %p, *p = %p", p, *p);
    return *p;
}

struct ThreadClosure
{
    void *(*start)(void *);
    void *restrict arg;
    void (*init)();
};

static void *ThreadMain(void *data)
{
    struct ThreadClosure *closure = data;
    closure->init();
    return closure->start(closure->arg);
}

int pthread_create(
    pthread_t *restrict thread,
    const pthread_attr_t *restrict _attr,
    void *(*start_routine)(void *),
    void *restrict arg)
{
    MESSAGE("start_routine = %p", start_routine);
    if (per_lcore__lcore_id != 0)
    {
        MESSAGE("not allowed in worker thread");
        abort();
    }
    return (interface.pthreadCreate)(thread, NULL, start_routine, arg);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    // TODO
    struct ThreadClosure *closure = (interface.malloc)(sizeof(struct ThreadClosure));
    closure->init = NULL; // TODO
    closure->start = start_routine;
    closure->arg = arg;
    return (interface.pthreadCreate)(thread, &attr, ThreadMain, closure);
}