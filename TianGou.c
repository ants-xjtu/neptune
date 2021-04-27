#include "TianGou.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>

Interface interface;

void exit(int stat)
{
    int sleeper;
    printf("exit(int) intercepted by Qcloud to see backtrace!\n");
    printf("input an integer and have a nice segfault! Don't worry, it's expected:)\n");
    scanf("%d", &sleeper);
}

void *malloc(size_t size)
{
    // TODO: manual SFI
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
    // int sleeper; // Qcloud's favorite way to see memory layout
    // scanf("%d", &sleeper);
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
            // MESSAGE("arena region:\t%#lx ..< %#lx", *interface.packetRegionLow, *interface.packetRegionHigh);
            // MESSAGE("start user callback at %p", callback);
            callback(user, &header, (u_char *)packet);
            // MESSAGE("%s", DONE_STRING);
        }
    }
    return 0;
}

int pcap_compile(pcap_t *p, struct bpf_program *fp,
                 const char *str, int optimize, bpf_u_int32 netmask)
{
    MESSAGE("return 0 by Qcloud");
    return 0;
}

char *default_dev = "eno1"; // modify this if things went wrong

char *pcap_lookupdev(char *errbuf)
{
    MESSAGE("lookupdev return default device");
    // strncpy(errbuf, "not supported", PCAP_ERRBUF_SIZE);
    return default_dev;
}

pcap_t *pcap_open_live(const char *device, int snaplen,
                       int promisc, int to_ms, char *errbuf)
{
    MESSAGE("return NULL, errbuf filled");
    // strncpy(errbuf, "not supported", PCAP_ERRBUF_SIZE);
    return (pcap_t *)1;
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

int	pcap_stats(pcap_t * p, struct pcap_stat * pt)
{
    MESSAGE("pcap check always pass");
    return 1;
}

/***    libc blacklist functions begin    ***/ 

char *strdup(const char *str)
{
    // toy version of strlen, and duplicate string on private heap
    const char *curr = str;
    size_t cnt = 0;
    while(*curr != '\0')
    {
        cnt++;
        curr++;
    }
    char *x = (char *)(interface.malloc)(cnt + 1);
    curr = str;
    char *cx = x;
    while(*curr != '\0')
    {
        *cx = *curr;
        cx++;
        curr++;
    }
    *cx = '\0';
    return x;
}

/***    vDPDK begins    ***/
int rte_eal_init(int argc, char **argv)
{
    MESSAGE("return 1(EAL already parsed)");
    return 1;
}

void rte_exit(int exit_code, const char *format, ...)
{
    va_list ap;
    fprintf(stderr, "[tiangou] rte_exit with exit_code %d: ", exit_code);
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    exit(exit_code);
}

uint16_t rte_eth_dev_count_avail()
{
    MESSAGE("return 2 ports");
    return 2;
}

struct rte_mempool *rte_pktmbuf_pool_create(const char *name, unsigned int n, 
        unsigned int cache_size, uint16_t priv_size, uint16_t data_room_size, 
        int socket_id)
{
    MESSAGE("return a trivial mempool pointer");
    return (struct rte_mempool *)1;
}

int rte_eth_dev_configure(uint16_t port_id, uint16_t nb_rx_queue, 
        uint16_t nb_tx_queue, const struct rte_eth_conf *eth_conf)
{
    MESSAGE("return 0");
    return 0;
}

int rte_eth_dev_adjust_nb_rx_tx_desc(uint16_t port_id, 
        uint16_t *nb_rx_desc, uint16_t *nb_tx_desc)
{
    MESSAGE("return 0");
    return 0;
}

int rte_eth_rx_queue_setup(uint16_t port_id, uint16_t rx_queue_id, 
        uint16_t nb_rx_desc, unsigned int socket_id, 
        const struct rte_eth_rxconf *rx_conf, struct rte_mempool *mb_pool)
{
    MESSAGE("rx_queue is previously initialized");
    return 0;
}

int rte_eth_tx_queue_setup(uint16_t port_id, uint16_t tx_queue_id, 
        uint16_t nb_tx_desc, unsigned int socket_id, 
        const struct rte_eth_txconf *tx_conf)
{
    MESSAGE("tx_queue is previously initialized");
    return 0;
}

void *rte_zmalloc_socket(const char *type, size_t size, unsigned int align, int socket)
{
    // all ports shared one txBuffer
    MESSAGE("zmalloc currently unimplemented");
    return NULL;
}

int
rte_eth_tx_buffer_init(struct rte_eth_dev_tx_buffer *buffer, uint16_t size)
{
    MESSAGE("return 0");
    return 0;
}

int rte_eth_tx_buffer_set_err_callback(struct rte_eth_dev_tx_buffer *buffer, 
        buffer_tx_error_fn callback, void *userdata)
{
    MESSAGE("return 0");
    return 0;
}

int rte_eth_dev_start(uint16_t port_id)
{
    MESSAGE("return 0(device already up)");
    return 0;
}

int rte_eth_promiscuous_enable(uint16_t port_id)
{
    MESSAGE("return 0(promisc mode on)");
    return 0;
}

int rte_eth_dev_stop(uint16_t port_id)
{
    MESSAGE("return 0(moon doesn't stop device)");
    return 0;
}

int rte_eth_dev_close(uint16_t port_id)
{
    MESSAGE("return 0(moon doesn't close device)");
    return 0;
}

/*  tx/rx control  */
int consumed = 0;
uint16_t rte_eth_rx_burst(uint16_t port_id, uint16_t queue_id, 
        struct rte_mbuf **rx_pkts, const uint16_t nb_pkts)
{
    if(!consumed)
    {
        consumed = 1;
        // Do we prefetch the packets here, or let the moon do this seemingly harmless job?
        rx_pkts = interface.packetBurst;
        return interface.burstSize;
    }
    else
    {
        consumed = 0;
        (interface.StackSwitch)(-1);
        return rte_eth_rx_burst(port_id, queue_id, rx_pkts, nb_pkts);
    }
}

uint16_t rte_eth_tx_buffer(uint16_t port_id, uint16_t queue_id, 
        struct rte_eth_dev_tx_buffer *buffer, struct rte_mbuf *tx_pkt)
{
    // MESSAGE("packet buffered into txBuffer in RunTime");
    // note that each call to tx_buffer will only buffer one mbuf, so we return 1
    return 1;
}

uint16_t rte_eth_tx_buffer_flush(uint16_t port_id, 
        uint16_t queue_id, struct rte_eth_dev_tx_buffer *buffer)
{
    // MESSAGE("flush singleton txBuffer");
    return 0;
}