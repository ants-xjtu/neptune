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

    struct rte_eth_dev_info *srcInfo, *dstInfo;
    uint64_t tscHz;
} Interface;

// the variable `interface` and `rte_eth_dev` is defined in TianGou.c
// because they must not be defined as extern

#endif