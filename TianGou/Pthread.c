#include "TianGou.h"

// void *__tls_get_addr(void **p)
// {
//     MESSAGE("p = %p, *p = %p", p, *p);
//     return *p;
// }

// int pthread_create(
//     pthread_t *restrict thread,
//     const pthread_attr_t *restrict _attr,
//     void *(*start_routine)(void *),
//     void *restrict arg)
// {
//     MESSAGE("start_routine = %p", start_routine);
//     if (per_lcore__lcore_id != 0)
//     {
//         MESSAGE("not allowed in worker thread");
//         abort();
//     }
//     return (interface.pthreadCreate)(thread, NULL, start_routine, arg);
// }

// int sched_setaffinity(pid_t pid, size_t cpusetsize, const cpu_set_t *mask)
// {
//     MESSAGE("pid = %d, cpusetsize = %lu, mask at %p", pid, cpusetsize, mask);
//     MESSAGE("return 0");
//     return 0;
// }

// int pthread_setaffinity_np(pthread_t thread, size_t cpusetsize,
//                                   const cpu_set_t *cpuset)
// {
//     MESSAGE("thread = %lu, cpusetsize = %lu, mask at %p", thread, cpusetsize, cpuset);
//     MESSAGE("return 0");
//     return 0;
// }

// int pthread_cond_timedwait(
//     pthread_cond_t *restrict cond,
//     pthread_mutex_t *restrict mutex,
//     const struct timespec *restrict abstime)
// {
//     MESSAGE("cond = %p, mutex = %p, abstime = .tv_sec = %ld, .tv_nsec = %ld", cond, mutex, abstime->tv_sec, abstime->tv_nsec);
//     return (interface.pthreadCondTimedwait)(cond, mutex, abstime);
// }

// int pthread_cond_wait(
//     pthread_cond_t *restrict cond,
//     pthread_mutex_t *restrict mutex)
// {
//     MESSAGE("cond = %p, mutex = %p", cond, mutex);
//     return (interface.pthreadCondWait)(cond, mutex);
// }