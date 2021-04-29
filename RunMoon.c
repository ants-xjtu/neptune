#define _ISOC11_SOURCE
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <dlfcn.h>
#include <unistd.h>
#include <time.h>

#include <rte_common.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_eal.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_string_fns.h>

#include "PrivateHeap.h"
#include "PrivateStack.h"
#include "PrivateLoad.h"
#include "TianGou.h"

const size_t STACK_SIZE = 32ul << 20; // 32MB
const size_t MOON_SIZE = 32ul << 30;  // 32GB
// Heap size = MOON_SIZE - STACK_SIZE - library.length

// we are doing a chain, so no static moon
// const int MOON_ID = 0;
const char *DONE_STRING = "\xe2\x86\x91 done";
#define RTE_TEST_RX_DESC_DEFAULT 1024
#define RTE_TEST_TX_DESC_DEFAULT 1024
static uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
static uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;

int (*moonStart)(int argc, char *argv[]);
void initMoon();
static volatile bool force_quit;
void l2fwd_main_loop(void);
void LoadMoon(char *, struct rte_mempool *, int);
int MoonNum;       //the total number of moons, temporary use
void *nf_state[2]; //the state information of the list of moon, temporary use

static void
signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM)
    {
        printf("\n\nSignal %d received, preparing to exit...\n",
               signum);
        force_quit = true;
    }
}

/* Check the link status of all ports in up to 9s, and print them finally */
static void
check_all_ports_link_status(uint32_t port_mask)
{
#define CHECK_INTERVAL 100 /* 100ms */
#define MAX_CHECK_TIME 90  /* 9s (90 * 100ms) in total */
    uint16_t portid;
    uint8_t count, all_ports_up, print_flag = 0;
    struct rte_eth_link link;
    int ret;
    char link_status_text[RTE_ETH_LINK_MAX_STR_LEN];

    printf("Checking link status\n");
    fflush(stdout);
    for (count = 0; count <= MAX_CHECK_TIME; count++)
    {
        if (force_quit)
            return;
        all_ports_up = 1;
        RTE_ETH_FOREACH_DEV(portid)
        {
            if (force_quit)
                return;
            if ((port_mask & (1 << portid)) == 0)
                continue;
            memset(&link, 0, sizeof(link));
            ret = rte_eth_link_get_nowait(portid, &link);
            if (ret < 0)
            {
                all_ports_up = 0;
                if (print_flag == 1)
                    printf("Port %u link get failed: %s\n",
                           portid, rte_strerror(-ret));
                continue;
            }
            /* print link status if flag set */
            if (print_flag == 1)
            {
                rte_eth_link_to_str(link_status_text,
                                    sizeof(link_status_text), &link);
                printf("Port %d %s\n", portid,
                       link_status_text);
                continue;
            }
            /* clear all_ports_up flag if any link down */
            if (link.link_status == ETH_LINK_DOWN)
            {
                all_ports_up = 0;
                break;
            }
        }
        /* after finally printing all link status, get out */
        if (print_flag == 1)
            break;

        if (all_ports_up == 0)
        {
            printf(".");
            fflush(stdout);
            rte_delay_ms(CHECK_INTERVAL);
        }

        /* set the print_flag if all ports up or timeout */
        if (all_ports_up == 1 || count == (MAX_CHECK_TIME - 1))
        {
            print_flag = 1;
            printf("%s\n", DONE_STRING);
        }
    }
}

struct l2fwd_port_statistics
{
    uint64_t tx;
    uint64_t rx;
    uint64_t dropped;
} __rte_cache_aligned;
struct l2fwd_port_statistics port_statistics;

struct rte_eth_dev_tx_buffer *txBuffer;
Interface *interface;
uint16_t srcPort, dstPort;

int main(int argc, char *argv[], char *envp[])
{
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");
    argc -= ret;
    argv += ret;

    force_quit = false;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    uint16_t nb_ports = rte_eth_dev_count_avail();
    if (nb_ports < 2)
        rte_exit(EXIT_FAILURE, "No enough Ethernet ports - bye\n");
    uint16_t portid;
    int portCount = 0;
    RTE_ETH_FOREACH_DEV(portid)
    {
        if (portCount == 0)
        {
            srcPort = portid;
        }
        else if (portCount == 1)
        {
            dstPort = portid;
        }
        portCount += 1;
    }
    printf("ethernet rx port: %u, tx port: %u\n", srcPort, dstPort);

    const unsigned int numberMbufs = 8192u, MEMPOOL_CACHE_SIZE = 256;
    struct rte_mempool *pktmbufPool = rte_pktmbuf_pool_create("mbuf_pool", numberMbufs, MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (pktmbufPool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");

    static struct rte_eth_conf port_conf = {
        .rxmode = {
            .split_hdr_size = 0,
        },
        .txmode = {
            .mq_mode = ETH_MQ_TX_NONE,
        },
    };

    RTE_ETH_FOREACH_DEV(portid)
    {
        if (portid != srcPort && portid != dstPort)
        {
            continue;
        }
        struct rte_eth_rxconf rxq_conf;
        struct rte_eth_txconf txq_conf;
        struct rte_eth_conf local_port_conf = port_conf;
        struct rte_eth_dev_info dev_info;
        struct rte_ether_addr ethernetAddress;

        printf("Initializing port %u...\n", portid);
        ret = rte_eth_dev_info_get(portid, &dev_info);
        if (ret != 0)
            rte_exit(EXIT_FAILURE,
                     "Error during getting device (port %u) info: %s\n",
                     portid, strerror(-ret));

        if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
            local_port_conf.txmode.offloads |=
                DEV_TX_OFFLOAD_MBUF_FAST_FREE;
        ret = rte_eth_dev_configure(portid, 1, 1, &local_port_conf);
        if (ret < 0)
            rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n",
                     ret, portid);

        ret = rte_eth_dev_adjust_nb_rx_tx_desc(portid, &nb_rxd,
                                               &nb_txd);
        if (ret < 0)
            rte_exit(EXIT_FAILURE,
                     "Cannot adjust number of descriptors: err=%d, port=%u\n",
                     ret, portid);

        ret = rte_eth_macaddr_get(portid, &ethernetAddress);
        if (ret < 0)
            rte_exit(EXIT_FAILURE,
                     "Cannot get MAC address: err=%d, port=%u\n",
                     ret, portid);

        /* init one RX queue */
        fflush(stdout);
        rxq_conf = dev_info.default_rxconf;
        rxq_conf.offloads = local_port_conf.rxmode.offloads;
        ret = rte_eth_rx_queue_setup(portid, 0, nb_rxd,
                                     rte_eth_dev_socket_id(portid),
                                     &rxq_conf,
                                     pktmbufPool);
        if (ret < 0)
            rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup:err=%d, port=%u\n",
                     ret, portid);

        /* init one TX queue on each port */
        fflush(stdout);
        txq_conf = dev_info.default_txconf;
        txq_conf.offloads = local_port_conf.txmode.offloads;
        ret = rte_eth_tx_queue_setup(portid, 0, nb_txd,
                                     rte_eth_dev_socket_id(portid),
                                     &txq_conf);
        if (ret < 0)
            rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup:err=%d, port=%u\n",
                     ret, portid);

        txBuffer = rte_zmalloc_socket("tx_buffer",
                                      RTE_ETH_TX_BUFFER_SIZE(MAX_PKT_BURST), 0,
                                      rte_eth_dev_socket_id(portid));
        if (txBuffer == NULL)
            rte_exit(EXIT_FAILURE, "Cannot allocate buffer for tx on port %u\n",
                     portid);

        rte_eth_tx_buffer_init(txBuffer, MAX_PKT_BURST);

        ret = rte_eth_tx_buffer_set_err_callback(txBuffer,
                                                 rte_eth_tx_buffer_count_callback,
                                                 &port_statistics.dropped);
        if (ret < 0)
            rte_exit(EXIT_FAILURE,
                     "Cannot set error callback for tx buffer on port %u\n",
                     portid);

        // This is the function left out by sgdxbc. Is it intentional?
        ret = rte_eth_dev_set_ptypes(portid, RTE_PTYPE_UNKNOWN, NULL,
                                     0);
        if (ret < 0)
            printf("Port %u, Failed to disable Ptype parsing\n",
                   portid);

        ret = rte_eth_dev_start(portid);
        if (ret < 0)
            rte_exit(EXIT_FAILURE, "rte_eth_dev_start:err=%d, port=%u\n",
                     ret, portid);

        printf("%s\n", DONE_STRING);

        ret = rte_eth_promiscuous_enable(portid);
        if (ret != 0)
            rte_exit(EXIT_FAILURE,
                     "rte_eth_promiscuous_enable:err=%s, port=%u\n",
                     rte_strerror(-ret), portid);

        printf("Port %u, MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
               portid,
               ethernetAddress.addr_bytes[0],
               ethernetAddress.addr_bytes[1],
               ethernetAddress.addr_bytes[2],
               ethernetAddress.addr_bytes[3],
               ethernetAddress.addr_bytes[4],
               ethernetAddress.addr_bytes[5]);
    }

    uint32_t portMask = 0;
    portMask |= (1 << srcPort);
    portMask |= (1 << dstPort);
    check_all_ports_link_status(portMask);

    if (argc < 3)
    {
        printf("usage: %s [EAL options] -- <tiangou> <moon>\n", argv[0]);
        return 0;
    }
    MoonNum = argc - 2;
    const char *tiangouPath = argv[1];
    InitLoader(argc, argv, envp);

    printf("loading tiangou library %s\n", tiangouPath);
    void *tiangou = dlopen(tiangouPath, RTLD_LAZY);
    interface = dlsym(tiangou, "interface");
    printf("injecting interface functions at %p\n", interface);
    interface->malloc = HeapMalloc;
    interface->realloc = HeapRealloc;
    interface->calloc = HeapCalloc;
    interface->free = HeapFree;
    interface->StackSwitch = StackSwitch;
    printf("HeapMalloc at address %p\n", HeapMalloc);
    printf("configure preloading for tiangou\n");
    PreloadLibrary(tiangou);
    printf("%s\n", DONE_STRING);

    for (int i = 2; i < argc; i++)
    {
        // we give each moon an id here, and it needs to be recycled later
        printf("loading moon%d...\n", i - 1);
        LoadMoon(argv[i], pktmbufPool, i - 2);
    }

    printf("*** START RUNTIME MAIN LOOP ***\n");
    l2fwd_main_loop();

    return 0;
}

void initMoon()
{
    char *argv[0];
    printf("[initMoon] calling moonStart\n");
    moonStart(0, argv);
    StackSwitch(-1);
    printf("[initMoon] nf exited\n");
    exit(0);
}

void LoadMoon(char *moonPath, struct rte_mempool *pktmbufPool, int MOON_ID)
{
    printf("allocating memory for MOON %s\n", moonPath);
    void *arena = aligned_alloc(MOON_SIZE, MOON_SIZE);
    printf("arena address %p size %#lx\n", arena, MOON_SIZE);

    struct PrivateLibrary library;
    library.file = moonPath;
    LoadLibrary(&library);
    printf("library requires space: %#lx\n", library.length);
    void *stackStart = arena, *libraryStart = arena + STACK_SIZE;
    void *heapStart = libraryStart + library.length;
    size_t heapSize = MOON_SIZE - STACK_SIZE - library.length;

    printf("*** MOON memory layout ***\n");
    printf("Stack\t%p - %p\tsize: %#lx\n", stackStart, stackStart + STACK_SIZE, STACK_SIZE);
    printf("Library\t%p - %p\tsize: %#lx\n", libraryStart, libraryStart + library.length, library.length);
    printf("Heap\t%p - %p\tsize: %#lx\n", heapStart, heapStart + heapSize, heapSize);
    printf("\n");

    printf("initializing regions\n");
    SetStack(MOON_ID, stackStart, STACK_SIZE);
    library.loadAddress = libraryStart;
    DeployLibrary(&library);
    SetHeap(heapStart, heapSize, MOON_ID);
    InitHeap();
    printf("%s\n", DONE_STRING);

    printf("setting protection region for MOON\n");
    uintptr_t *mainPrefix = LibraryFind(&library, "SwordHolder_MainPrefix");
    *mainPrefix = (uintptr_t)arena;
    printf("main region prefix:\t%#lx (align 32GB)\n", *mainPrefix);
    interface->packetRegionLow = LibraryFind(&library, "SwordHolder_ExtraLow");
    interface->packetRegionHigh = LibraryFind(&library, "SwordHolder_ExtraHigh");
    // below is a quick implement for 1 prot region, comment 2 lines if you are doing 2 regions
    // *gmemLowRegionLow = (uintptr_t)arena;
    // *gmemLowRegionHigh = *gmemLowRegionLow + MOON_SIZE;

    nf_state[MOON_ID] = LibraryFind(&library, "state");

    moonStart = LibraryFind(&library, "main");
    printf("entering MOON for initial running, start = %p\n", moonStart);
    StackStart(MOON_ID, initMoon);
    printf("%s\n", DONE_STRING);
}

static void
print_stats(void)
{
    printf(
        "Packets sent: %24" PRIu64
        " Packets received: %20" PRIu64
        " Packets dropped: %21" PRIu64 "\n",
        port_statistics.tx,
        port_statistics.rx,
        port_statistics.dropped);

    fflush(stdout);
}

#define BURST_TX_DRAIN_US 100 /* TX drain every ~100us */
#define MEMPOOL_CACHE_SIZE 256
#define RTE_LOGTYPE_L2FWD RTE_LOGTYPE_USER1

static uint64_t timer_period = 5; /* default period is 10 seconds */

void l2fwd_main_loop(void)
{
    int sent;
    unsigned lcore_id;
    uint64_t prev_tsc, diff_tsc, cur_tsc, timer_tsc;
    unsigned i, j, portid, nb_rx;
    const uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S *
                               BURST_TX_DRAIN_US;
    struct rte_eth_dev_tx_buffer *buffer;
    uint64_t prev_pkts, cur_pkts;
    uint64_t prev_bytes, cur_bytes;
    struct timeval ts, te;
    int pkt_interval = 0;

    prev_tsc = 0;
    timer_tsc = 0;
    prev_pkts = 0;
    cur_pkts = 0;
    prev_bytes = 0;
    cur_bytes = 0;
    gettimeofday(&ts, NULL);

    lcore_id = rte_lcore_id();
    timer_period *= rte_get_timer_hz();

    RTE_LOG(INFO, L2FWD, "entering main loop on lcore %u\n", lcore_id);

    while (!force_quit)
    {

        cur_tsc = rte_rdtsc();
        cur_pkts = port_statistics.rx;

        /*
		 * TX burst queue drain
		 */
        diff_tsc = cur_tsc - prev_tsc;
        if (unlikely(diff_tsc > drain_tsc))
        {
            portid = dstPort;
            buffer = txBuffer;

            sent = rte_eth_tx_buffer_flush(portid, 0, buffer);
            if (sent)
                port_statistics.tx += sent;

            /* if timer is enabled */
            if (timer_period > 0)
            {

                /* advance the timer */
                timer_tsc += diff_tsc;

                /* if timer has reached its timeout */
                if (unlikely(timer_tsc >= timer_period))
                {

                    /* do this only on main core */
                    if (lcore_id == rte_get_main_lcore())
                    {
                        /* reset the timer */
                        timer_tsc = 0;
                        gettimeofday(&te, NULL);
                        time_t s = te.tv_sec - ts.tv_sec;
                        suseconds_t u = s * 1000000 + te.tv_usec - ts.tv_usec;
                        double throughput = (double)(cur_bytes - prev_bytes) * 8 / u;
                        printf("%f\n", throughput);
                        prev_bytes = cur_bytes;
                        prev_pkts = cur_pkts;
                        gettimeofday(&ts, NULL);
                    }
                }
            }

            prev_tsc = cur_tsc;
        }

        /*
		 * Read packet from RX queues
		 */
        portid = srcPort;
        // potential problems here: are all the MOONs share the same packetBurst inside interface?
        nb_rx = rte_eth_rx_burst(portid, 0, interface->packetBurst, MAX_PKT_BURST);

        port_statistics.rx += nb_rx;
        interface->burstSize = nb_rx;
        pkt_interval += nb_rx;
        for (int i = 0; i < MoonNum; i++)
        {
            HeapSwitch(i);
            StackSwitch(i);
        }
        for (i = 0; i < nb_rx; i++)
        {
            struct rte_mbuf *m = interface->packetBurst[i];
            rte_prefetch0(rte_pktmbuf_mtod(m, void *));
            sent = rte_eth_tx_buffer(dstPort, 0, txBuffer, m);
            if (sent)
                port_statistics.tx += sent;
            // pkt len or data len?
            cur_bytes += rte_pktmbuf_pkt_len(m);
        }
    }
    for (int i = 0; i < MoonNum; i++)
    {
        if (nf_state[i])
        {
            printf("print state maintained by moon %d\n", i);
            int *real_state = nf_state[i];
            printf("unrecognized packets: %d\n", *real_state);
            printf("setting up connections: %d\n", *(real_state + 1));
            printf("closing connections: %d\n", *(real_state + 2));
            printf("resetting connections: %d\n", *(real_state + 3));
            printf("real data: %d\n\n", *(real_state + 4));
        }
    }
    printf("total bytes processed: %21" PRIu64 "\n", cur_bytes);
    printf("total packets processed: %21" PRIu64 "\n", cur_pkts);
    printf(" Packets dropped: %21" PRIu64 "\n", port_statistics.dropped);
}