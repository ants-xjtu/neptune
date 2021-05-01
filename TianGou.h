#ifndef NEPTUNE_TIANGOU_H
#define NEPTUNE_TIANGOU_H

#define _GNU_SOURCE
#include <stddef.h>
#include <pcap/pcap.h>
#include <rte_mbuf.h>

typedef struct
{
    void *(*malloc)(size_t);
    void *(*realloc)(void *, size_t);
    void *(*calloc)(size_t, size_t);
    void (*free)(void *);

    int (*pcapLoop)(pcap_t *, int, pcap_handler, u_char *);
} Interface;

#endif