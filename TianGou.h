#ifndef NEPTUNE_TIANGOU_H
#define NEPTUNE_TIANGOU_H

#define _GNU_SOURCE
#include <signal.h>
#include <stddef.h>
#include <pcap/pcap.h>
#include <rte_ethdev.h>
#include <rte_lpm.h>

typedef struct
{
    void *(*malloc)(size_t);
    void *(*realloc)(void *, size_t);
    void *(*calloc)(size_t, size_t);
    void (*free)(void *);
    void *(*alignedAlloc)(size_t, size_t);

    sighandler_t (*signal)(int signum, sighandler_t handler);
    int (*sigaction)(int signum, const struct sigaction *restrict act,
                     struct sigaction *restrict oldact);

    int (*pcapLoop)(pcap_t *, int, pcap_handler, u_char *);
    const u_char *(*pcapNext)(pcap_t *p, struct pcap_pkthdr *h);
    int (*pcapDispatch)(pcap_t *p, int cnt, pcap_handler callback, u_char *user);

    // this two could be function pointer
    // but they always return constant, so value is better
    struct rte_eth_dev_info *srcInfo, *dstInfo;
    uint64_t tscHz;
    // temporary fix for uninitialized lpm functions
    struct rte_lpm *(*lpm_create)(const char *name, int socket_id,
		const struct rte_lpm_config *config);
    struct rte_lpm *(*lpm_find_existing)(const char *name);
    int (*lpm_add)(struct rte_lpm *lpm, uint32_t ip, uint8_t depth, uint32_t next_hop);


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