#include "TianGou.h"
#include <stdio.h>
#include <string.h>

Interface interface;

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

#define MESSAGE0() MESSAGE1("")
#define MESSAGE1(extra_str) printf("[tiangou] %s " extra_str "\n", __func__)
#define MESSAGE_(extra_str, extra_args...) printf("[tiangou] %s: " extra_str "\n", __func__, ##extra_args)
#define GET_MACRO(_0, _1, _2, _3, _4, _5, _6, _7, NAME, ...) NAME
#define MESSAGE(...)                                                                                             \
    GET_MACRO(_0, ##__VA_ARGS__, MESSAGE_, MESSAGE_, MESSAGE_, MESSAGE_, MESSAGE_, MESSAGE_, MESSAGE1, MESSAGE0) \
    (__VA_ARGS__)

const char *DONE_STRING = "\xe2\x86\x91 done";

int pcap_setfilter(pcap_t *p, struct bpf_program *fp)
{
    MESSAGE();
    return 0;
}

const u_char *pcap_next(pcap_t *p, struct pcap_pkthdr *h)
{
    MESSAGE("return NULL");
    return NULL;
}

int pcap_get_selectable_fd(pcap_t *p)
{
    MESSAGE("return -1");
    return -1;
}

void pcap_close(pcap_t *p)
{
    MESSAGE();
}

char *pcap_geterr(pcap_t *p)
{
    MESSAGE("return \"you are in a MOON\"");
    return "you are in a MOON";
}

pcap_t *pcap_open_offline(const char *fname, char *errbuf)
{
    MESSAGE("fname = %s", fname);
    return (pcap_t *)0x42;
}

int pcap_loop(pcap_t *p, int cnt, pcap_handler callback, u_char *user)
{
    MESSAGE("callback = %p\n", callback);
    for (;;)
    {
        (interface.StackSwitch)(-1);
        for (int i = 0; i < interface.burstSize; i += 1)
        {
            rte_prefetch0(rte_pktmbuf_mtod(interface.packetBurst[i], void *));
            struct pcap_pkthdr header;
            header.len = rte_pktmbuf_pkt_len(interface.packetBurst[i]);
            header.caplen = rte_pktmbuf_data_len(interface.packetBurst[i]);
            memset(&header.ts, 0x0, sizeof(header.ts)); // todo
            uintptr_t *packet = rte_pktmbuf_mtod(interface.packetBurst[i], uintptr_t *);
            *interface.packetRegionLow = (uintptr_t)packet;
            *interface.packetRegionHigh = *interface.packetRegionLow + header.caplen;
            MESSAGE("arena region:\t%#lx ..< %#lx", *interface.packetRegionLow, *interface.packetRegionHigh);
            MESSAGE("start user callback at %p", callback);
            callback(user, &header, (u_char *)packet);
            MESSAGE("%s", DONE_STRING);
        }
    }
    return 0;
}

int pcap_compile(pcap_t *p, struct bpf_program *fp,
                 const char *str, int optimize, bpf_u_int32 netmask)
{
    MESSAGE("return -1");
    return -1;
}

char *pcap_lookupdev(char *errbuf)
{
    MESSAGE("return NULL, errbuf filled");
    strncpy(errbuf, "not supported", PCAP_ERRBUF_SIZE);
    return NULL;
}

pcap_t *pcap_open_live(const char *device, int snaplen,
                       int promisc, int to_ms, char *errbuf)
{
    MESSAGE("return NULL, errbuf filled");
    strncpy(errbuf, "not supported", PCAP_ERRBUF_SIZE);
    return NULL;
}

int pcap_datalink(pcap_t *p)
{
    MESSAGE("return DLT_EN10MB");
    return DLT_EN10MB;
}

int pcap_dispatch(pcap_t *p, int cnt, pcap_handler callback, u_char *user)
{
    MESSAGE("return -1");
    return -1;
}