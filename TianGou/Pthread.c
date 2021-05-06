#include "TianGou.h"

void *__tls_get_addr(void **p)
{
    MESSAGE("p = %p, *p = %p", p, *p);
    return *p;
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
}

int sched_setaffinity(pid_t pid, size_t cpusetsize, const cpu_set_t *mask)
{
    MESSAGE("pid = %d, cpusetsize = %lu, mask at %p", pid, cpusetsize, mask);
    MESSAGE("return 0");
    return 0;
}