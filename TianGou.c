#include "TianGou.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <rte_mempool.h>

#define MESSAGE0() MESSAGE1("")
#define MESSAGE1(extra_str) printf("[tiangou] %s " extra_str "\n", __func__)
#define MESSAGE_(extra_str, extra_args...) printf("[tiangou] %s: " extra_str "\n", __func__, ##extra_args)
#define GET_MACRO(_0, _1, _2, _3, _4, _5, _6, _7, NAME, ...) NAME
#define MESSAGE(...)                                                                                             \
    GET_MACRO(_0, ##__VA_ARGS__, MESSAGE_, MESSAGE_, MESSAGE_, MESSAGE_, MESSAGE_, MESSAGE_, MESSAGE_, MESSAGE0) \
    (__VA_ARGS__)

const char *DONE_STRING = "\xe2\x86\x91 done";

Interface interface;

// malloc
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

// crucial part of glibc
void exit(int stat)
{
    MESSAGE("nf recovering is not implemented");
    abort();
}

sighandler_t signal(int signum, sighandler_t handler)
{
    MESSAGE("nf try to set handler for signal %d", signum);
    if (signum != SIGINT && signum != SIGTERM)
    {
        return (interface.signal)(signum, handler);
    }
    return NULL;
}

char *strdup(const char *str)
{
    // toy version of strlen, and duplicate string on private heap
    const char *curr = str;
    size_t cnt = 0;
    while (*curr != '\0')
    {
        cnt++;
        curr++;
    }
    char *x = (char *)(interface.malloc)(cnt + 1);
    curr = str;
    char *cx = x;
    while (*curr != '\0')
    {
        *cx = *curr;
        cx++;
        curr++;
    }
    *cx = '\0';
    return x;
}

// pcap
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
    MESSAGE("callback = %p", callback);
    return (interface.pcapLoop)(p, cnt, callback, user);
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

int pcap_stats(pcap_t *p, struct pcap_stat *pt)
{
    MESSAGE("pcap check always pass");
    return 1;
}

// dpdk

// two devices for src/dst ports
// wrapper for rx/tx burst directly goes into runtime and reigster on this
// array, because the functions are inlined in DPDK
// this cannot go into interface, because I cannot "sync" it when assign to
// fields in interface
struct rte_eth_dev rte_eth_devices[2];

// __thread unsigned per_lcore__lcore_id;

int rte_eal_init(int argc, char **argv)
{
    MESSAGE("return 0");
    return 0;
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

// DPDK VM: 2 ports (#0 and #1), where all packets comes from
// #0 rx, and all packets send to #1 tx will be actaully sent
// send to #0 tx is no-op, and #1 rx alwasy empty
// the MAC addresses of two ports are ff:ff:ff:ff:ff:00/01
uint16_t rte_eth_dev_count_avail()
{
    MESSAGE("return 2");
    return 2;
}

uint64_t rte_eth_find_next_owned_by(uint16_t port_id, const uint64_t owner_id)
{
    MESSAGE("port_id = %u", port_id);
    return port_id > 1 ? RTE_MAX_ETHPORTS : port_id;
}

// DPDK VM: only 1 lcore #0 on socket #0 is available
int rte_lcore_is_enabled(unsigned int lcore_id)
{
    MESSAGE("lcore_id = %u", lcore_id);
    return lcore_id == 0;
}

unsigned int rte_socket_id(void)
{
    MESSAGE("return 0");
    return 0;
}

struct rte_mempool *rte_pktmbuf_pool_create(
    const char *name, unsigned int n,
    unsigned int cache_size, uint16_t priv_size, uint16_t data_room_size,
    int socket_id)
{
    MESSAGE("return 0x42");
    return (struct rte_mempool *)0x42;
}

void *rte_zmalloc_socket(const char *type, size_t size, unsigned int align, int socket)
{
    MESSAGE("type = %s, size = %lu, align = %u, socket = %d", type, size, align, socket);
    if (align != 0)
    {
        MESSAGE("not implemented");
        return NULL;
    }
    return (interface.malloc)(size);
}

// const struct rte_memzone *rte_memzone_reserve(
//     const char *name, size_t len, int socket_id, unsigned flags)
// {
//     MESSAGE("name = %s, len = %lu, socket_id = %d, flags = %u", name, len, socket_id, flags);
//     struct rte_memzone *mz = (interface.malloc)(sizeof(struct rte_memzone));
//     if (!mz)
//     {
//         return NULL;
//     }
//     void *mzAddr = (interface.malloc)(len);
//     if (!mzAddr)
//     {
//         (interface.free)(mz);
//         return NULL;
//     }
//     mz->addr = mzAddr;
//     mz->iova = (uint64_t)mzAddr;
//     mz->len = len;
//     mz->hugepage_sz = sysconf(_SC_PAGESIZE);
//     mz->socket_id = 0;
//     mz->flags = 0;
//     MESSAGE("return memzone = %p, mz->addr = %p", mz, mz->addr);
//     return mz;
// }

int rte_eth_dev_info_get(uint16_t port_id, struct rte_eth_dev_info *dev_info)
{
    if (port_id > 1)
    {
        return -ENODEV;
    }
    struct rte_eth_dev_info *info = port_id == 0 ? interface.srcInfo : interface.dstInfo;
    memcpy(dev_info, info, sizeof(struct rte_eth_dev_info));
    return 0;
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

int rte_eth_macaddr_get(uint16_t port_id, struct rte_ether_addr *mac_addr)
{
    for (int i = 0; i < 5; i += 1)
    {
        mac_addr->addr_bytes[i] = 0xff;
    }
    mac_addr->addr_bytes[5] = port_id;
    return 0;
}

int rte_eth_rx_queue_setup(uint16_t port_id, uint16_t rx_queue_id,
                           uint16_t nb_rx_desc, unsigned int socket_id,
                           const struct rte_eth_rxconf *rx_conf, struct rte_mempool *mb_pool)
{
    MESSAGE("return 0");
    return 0;
}

int rte_eth_tx_queue_setup(uint16_t port_id, uint16_t tx_queue_id,
                           uint16_t nb_tx_desc, unsigned int socket_id,
                           const struct rte_eth_txconf *tx_conf)
{
    MESSAGE("return 0");
    return 0;
}

int rte_eth_tx_buffer_init(struct rte_eth_dev_tx_buffer *buffer, uint16_t size)
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
    MESSAGE("return 0");
    return 0;
}

int rte_eth_promiscuous_enable(uint16_t port_id)
{
    MESSAGE("return 0");
    return 0;
}

int rte_eth_dev_stop(uint16_t port_id)
{
    MESSAGE("return 0");
    return 0;
}

int rte_eth_dev_close(uint16_t port_id)
{
    MESSAGE("return 0");
    return 0;
}

int rte_eal_mp_remote_launch(
    lcore_function_t *f, void *arg,
    enum rte_rmt_call_main_t call_main)
{
    MESSAGE("f = %p, arg = %p, call_main = %d", f, arg, call_main);
    if (call_main == CALL_MAIN)
    {
        return f(arg);
    }
    return 0;
}

unsigned int rte_get_next_lcore(unsigned int i, int skip_main, int wrap)
{
    MESSAGE("i = %u, skip_main = %d, wrap = %d", i, skip_main, wrap);
    unsigned int next = i == 0 && !skip_main ? 0 : RTE_MAX_LCORE;
    if (wrap && next == RTE_MAX_LCORE)
    {
        next = 0;
    }
    return next;
}

int rte_eal_wait_lcore(unsigned worker_id)
{
    MESSAGE("worker_id = %u", worker_id);
    return 0;
}

uint64_t rte_get_tsc_hz(void) {
    return interface.tscHz;
}
