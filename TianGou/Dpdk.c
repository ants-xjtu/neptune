#include "TianGou.h"
#include <unistd.h>

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

struct rte_mempool *rte_pktmbuf_pool_create(
    const char *name, unsigned int n,
    unsigned int cache_size, uint16_t priv_size, uint16_t data_room_size,
    int socket_id)
{
    MESSAGE("return 0x42");
    return (struct rte_mempool *)0x42;
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
    // MESSAGE("port_id = %u", port_id);
    return port_id > 1 ? RTE_MAX_ETHPORTS : port_id;
}

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

int rte_eth_dev_socket_id(uint16_t port_id)
{
    MESSAGE("return 0");
    return 0;
}

int rte_eth_macaddr_get(uint16_t port_id, struct rte_ether_addr *mac_addr)
{
    MESSAGE("port_id = %u", port_id);
    for (int i = 0; i < 5; i += 1)
    {
        mac_addr->addr_bytes[i] = 0xff;
    }
    mac_addr->addr_bytes[5] = port_id;
    return 0;
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

uint64_t rte_get_tsc_hz(void)
{
    return interface.tscHz;
}

int rte_log(uint32_t level, uint32_t logtype, const char *format, ...)
{
    MESSAGE("level = %u, logtype = %u, says:", level, logtype);
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
    return 0;
}

const struct rte_memzone *
rte_memzone_reserve_aligned(
    const char *name, size_t len, int socket_id,
    unsigned flags, unsigned align)
{
    MESSAGE("name = %s, len = %lu, socket_id = %d, flags = %u, align = %u", name, len, socket_id, flags, align);
    struct rte_memzone *mz = (interface.malloc)(sizeof(struct rte_memzone));
    strncpy(mz->name, name, RTE_MEMZONE_NAMESIZE);
    // mz->iova = RTE_BAD_IOVA;
    mz->addr = (interface.alignedAlloc)(align, len);
    mz->iova = (rte_iova_t)mz->addr;
    if (!mz->addr)
    {
        abort();
    }
    mz->len = len;
    mz->hugepage_sz = sysconf(_SC_PAGESIZE);
    mz->socket_id = 0;
    mz->flags = 0;
    return mz;
}

const struct rte_memzone *
rte_memzone_reserve(
    const char *name, size_t len, int socket_id,
    unsigned flags)
{
    return rte_memzone_reserve_aligned(name, len, socket_id, flags, 0);
}

// nop below here
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
    MESSAGE("create a one-sized buffer to prevent buffer");
    buffer->size = 1;
    buffer->length = 0;
    buffer->error_callback = NULL;
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

unsigned int rte_lcore_count()
{
    MESSAGE("return 1");
    return 1;
}

int rte_mbuf_dyn_rx_timestamp_register(int *field_offset, uint64_t *rx_flag)
{
    MESSAGE("return 0");
    return 0;
}

int rte_eth_dev_set_vlan_offload(uint16_t port_id, int offload_mask)
{
    MESSAGE("return 0");
    return 0;
}

int rte_eth_dev_is_valid_port(uint16_t port_id)
{
    MESSAGE("always valid");
    return 1;
}

int rte_flow_isolate(uint16_t port_id, int set, struct rte_flow_error *error)
{
    MESSAGE("return 0");
    return 0;
}

int rte_eth_dev_set_ptypes(uint16_t port_id, uint32_t ptype_mask,
			   uint32_t *set_ptypes, unsigned int num)
{
    MESSAGE("return 0");
    return 0;
}

int rte_eth_link_get_nowait (uint16_t port_id, struct rte_eth_link *link)
{
    // MESSAGE("return 0");
    return 0;
}

// WARNING: this is for debugging only
// void rte_mempool_free(struct rte_mempool *mp)
// {
//     MESSAGE("nor freeing when debugging");
//     return;
// }
int rte_dev_probe(const char *devargs)
{
    MESSAGE("return 0");
    return 0;
}
