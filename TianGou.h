#ifndef NEPTUNE_TIANGOU_H
#define NEPTUNE_TIANGOU_H

#define _GNU_SOURCE
#include <signal.h>
#include <stddef.h>
#include <pcap/pcap.h>
#include <rte_ethdev.h>

typedef struct
{
    void *(*malloc)(size_t);
    void *(*realloc)(void *, size_t);
    void *(*calloc)(size_t, size_t);
    void (*free)(void *);

    sighandler_t (*signal)(int signum, sighandler_t handler);

    int (*pcapLoop)(pcap_t *, int, pcap_handler, u_char *);
    const u_char *(*pcapNext)(pcap_t *p, struct pcap_pkthdr *h);

    // this two could be function pointer
    // but they always return constant, so value is better
    struct rte_eth_dev_info *srcInfo, *dstInfo;
    uint64_t tscHz;

    int (*pthreadCreate)(
        pthread_t *restrict thread,
        const pthread_attr_t *restrict attr,
        void *(*start_routine)(void *),
        void *restrict arg);
    int (*pthreadCondTimedwait)(
        pthread_cond_t *restrict cond,
        pthread_mutex_t *restrict mutex,
        const struct timespec *restrict abstime);
    int (*pthreadCondWait)(
        pthread_cond_t *restrict cond,
        pthread_mutex_t *restrict mutex);
} Interface;

extern Interface interface;

#define MESSAGE0() MESSAGE1("")
#define MESSAGE1(extra_str) printf("[tiangou] %s " extra_str "\n", __func__)
#define MESSAGE_(extra_str, extra_args...) printf("[tiangou] %s: " extra_str "\n", __func__, ##extra_args)
#define GET_MACRO(_0, _1, _2, _3, _4, _5, _6, _7, NAME, ...) NAME
#define MESSAGE(...)                                                                                             \
    GET_MACRO(_0, ##__VA_ARGS__, MESSAGE_, MESSAGE_, MESSAGE_, MESSAGE_, MESSAGE_, MESSAGE_, MESSAGE_, MESSAGE0) \
    (__VA_ARGS__)

static const char *DONE_STRING = "\xe2\x86\x91 done";

#endif