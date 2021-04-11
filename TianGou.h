#ifndef NEPTUNE_TIANGOU_H
#define NEPTUNE_TIANGOU_H

#define _GNU_SOURCE
#include <stddef.h>
#include <pcap/pcap.h>
#include <rte_mbuf.h>

#define MAX_PKT_BURST 32

typedef struct
{
    void *(*malloc)(size_t);
    void *(*realloc)(void *, size_t);
    void *(*calloc)(size_t, size_t);
    void (*free)(void *);

    void (*StackSwitch)(int);

    struct rte_mbuf *packetBurst[MAX_PKT_BURST];
    int burstSize;
} Interface;

typedef void (*pcap_handler)(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes);

#endif