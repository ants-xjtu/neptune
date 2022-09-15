#include "Common.h"

void signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM)
    {
        fflush(stdout);
        fflush(stderr);
        if (force_quit)
        {
            printf("Exit ungracefully now, should check for blocking/looping in code.\n");
            abort();
        }
        printf("Signal %d received, waiting for user action...\n\
Pressing \'d\' to start dumping, otherwise quitting\n", signum);
        char userOption = getchar();
        if (userOption == 'd')
        {
            printf("Confirm dumping.\n");
            need_dump = 1;
        }
        else
        {
            printf("receiving option %c\n", userOption);
            force_quit = true;
        }
            
    }
}

int dirty_lb = 0;
int dirty_mb = 0;
int dirty_ub = 0;
int iteration_epoch = 0;
struct dirtyPage dirtyBuffer[MAX_DIRTY_PAGE];

void segv_handler(int signum, siginfo_t *info, void *context)
{
    size_t addr_num = ((size_t)info->si_addr >> 12) << 12;
    // printf("Instruction pointer: %p\n",
    //        (((ucontext_t*)context)->uc_mcontext.gregs[16]));
    // printf("Addr: %p\n", (void *)addr_num);
    // note that printing in signal handler is generally bad approach

    dirtyBuffer[dirty_ub % MAX_DIRTY_PAGE].addr = addr_num;
    dirtyBuffer[dirty_ub % MAX_DIRTY_PAGE].len  = 4096;
    dirtyBuffer[dirty_ub % MAX_DIRTY_PAGE].iter = iteration_epoch;

    printf("[worker] catch a segfault in %p! Now dirty page count: %d\n", (void *)addr_num, dirty_ub - dirty_mb);
    if (mprotect((void *)addr_num, 4096, PROT_READ | PROT_WRITE))
    {
        printf("restoring memory protection failed\n");
        abort();
    }
    dirty_ub++;
}

// when the worker finds too many dirty pages, it blocks and retrieve the 'w' granted to pervious pages
void retrieve_perm()
{
    for (int i = dirty_mb; i < dirty_ub; i++)
    {
        if (unlikely(mprotect((void *)(dirtyBuffer[i].addr), dirtyBuffer[i].len, PROT_READ) == -1))
        {
            printf("[%s] cannot change permission at %lx, length: %u\n", __func__, dirtyBuffer[i].addr, dirtyBuffer[i].len);
            abort();
        }
    }
    // a new round of iteration lead to changes in dump prefix
    iteration_epoch++;
    dirty_mb = dirty_ub;
    printf("new round of iteration! Now at #%d\n", iteration_epoch);
}

/* Check the link status of all ports in up to 9s, and print them finally */
void check_all_ports_link_status(uint32_t port_mask)
{
#define CHECK_INTERVAL 100 /* 100ms */
// #define MAX_CHECK_TIME 90  /* 9s (90 * 100ms) in total */
#define MAX_CHECK_TIME 10
    uint16_t portid;
    uint8_t count, all_ports_up, print_flag = 0;
    struct rte_eth_link link;
    int ret;
    char link_status_text[RTE_ETH_LINK_MAX_STR_LEN];

    printf("Checking link status");
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
            printf("\n%s\n", DONE_STRING);
        }
    }
}

// #define NB_MBUFS 64 * 1024 /* use 64k mbufs */
#define NB_MBUFS 1024 * 1024
#define MBUF_CACHE_SIZE 256
#define PKT_BURST 32
#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

static size_t NumberQueue;

static uint8_t seed[40] = {
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A
};

static inline int
smp_port_init(uint16_t port, struct rte_mempool *mbuf_pool,
              uint16_t num_queues)
{
    NumberQueue = num_queues;

    struct rte_eth_conf port_conf = {
        .rxmode = {
            .mq_mode = ETH_MQ_RX_RSS,
            .split_hdr_size = 0,
            .offloads = DEV_RX_OFFLOAD_CHECKSUM,
        },
        .rx_adv_conf = {
            .rss_conf = {
                // .rss_key = NULL,
                // .rss_hf = ETH_RSS_IP,
                // .rss_hf = ETH_RSS_TCP,
                .rss_key = seed,
                .rss_hf = ETH_RSS_IP | ETH_RSS_UDP | ETH_RSS_TCP | ETH_RSS_L2_PAYLOAD,
                .rss_key_len = sizeof(seed)
            },
        },
        .txmode = {
            .mq_mode = ETH_MQ_TX_NONE,
        }};
    const uint16_t rx_rings = num_queues, tx_rings = num_queues;
    struct rte_eth_dev_info info;
    struct rte_eth_rxconf rxq_conf;
    struct rte_eth_txconf txq_conf;
    int retval;
    uint16_t q;
    uint16_t nb_rxd = RX_RING_SIZE;
    uint16_t nb_txd = TX_RING_SIZE;
    uint64_t rss_hf_tmp;

    if (rte_eal_process_type() == RTE_PROC_SECONDARY)
        return 0;

    if (!rte_eth_dev_is_valid_port(port))
        return -1;

    printf("# Initialising port %u... \n", port);
    printf("# Rx Queue: %u, Tx Queue: %u \n", rx_rings, tx_rings);
    fflush(stdout);

    retval = rte_eth_dev_info_get(port, &info);
    if (retval != 0)
    {
        printf("Error during getting device (port %u) info: %s\n",
               port, strerror(-retval));
        return retval;
    }

    info.default_rxconf.rx_drop_en = 1;

    if (info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
        port_conf.txmode.offloads |=
            DEV_TX_OFFLOAD_MBUF_FAST_FREE;

    rss_hf_tmp = port_conf.rx_adv_conf.rss_conf.rss_hf;
    port_conf.rx_adv_conf.rss_conf.rss_hf &= info.flow_type_rss_offloads;
    if (port_conf.rx_adv_conf.rss_conf.rss_hf != rss_hf_tmp)
    {
        printf("Port %u modified RSS hash function based on hardware support,"
               "requested:%#" PRIx64 " configured:%#" PRIx64 "\n",
               port,
               rss_hf_tmp,
               port_conf.rx_adv_conf.rss_conf.rss_hf);
    }

    retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
    if (retval < 0)
        return retval;

    retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
    if (retval < 0)
        return retval;

    rxq_conf = info.default_rxconf;
    rxq_conf.offloads = port_conf.rxmode.offloads;
    for (q = 0; q < rx_rings; q++)
    {
        retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
                                        rte_eth_dev_socket_id(port),
                                        &rxq_conf,
                                        mbuf_pool);
        if (retval < 0)
            return retval;
    }

    txq_conf = info.default_txconf;
    txq_conf.offloads = port_conf.txmode.offloads;
    for (q = 0; q < tx_rings; q++)
    {
        retval = rte_eth_tx_queue_setup(port, q, nb_txd,
                                        rte_eth_dev_socket_id(port),
                                        &txq_conf);
        if (retval < 0)
            return retval;
    }

    retval = rte_eth_promiscuous_enable(port);
    if (retval != 0)
        return retval;

    retval = rte_eth_dev_start(port);
    if (retval < 0)
        return retval;

    return 0;
}

struct rte_mempool *copyPool;

void SetupDpdk()
{
    int ret;

    force_quit = false;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // only register sigfault handler in source process, because there might be issues in dest.
    if (needMap == 0)
    {
        struct sigaction act1;
        act1.sa_flags = SA_SIGINFO | SA_NODEFER;
        act1.sa_sigaction = &segv_handler;
        sigaction(SIGSEGV, &act1, NULL);
    }
    
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
            // modified to use the same port for rx/tx
            dstPort = portid;
        }
        // else if (portCount == 1)
        // {
        //     dstPort = portid;
        // }
        portCount += 1;
    }
    printf("ethernet rx port: %u, tx port: %u\n", srcPort, dstPort);

    // const unsigned int numberMbufs = 8192u, MEMPOOL_CACHE_SIZE = 256;
    struct rte_mempool *pktmbufPool = rte_pktmbuf_pool_create("mbuf_pool", NB_MBUFS, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (pktmbufPool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");
    copyPool = rte_pktmbuf_pool_create("copy_pool", NB_MBUFS, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    // it is hard to determine the corrent value for them...
    // TODO
    mbufLow = 0;
    mbufHigh = ~mbufLow;

    RTE_ETH_FOREACH_DEV(portid)
    {
        if (portid != srcPort && portid != dstPort)
        {
            continue;
        }
        if (smp_port_init(portid, pktmbufPool, rte_lcore_count() - 1) < 0)
            rte_exit(EXIT_FAILURE, "Error initialising ports\n");
    }

    // txBuffer = rte_zmalloc_socket(
    //     "tx_buffer",
    //     RTE_ETH_TX_BUFFER_SIZE(MAX_PKT_BURST), 0,
    //     rte_eth_dev_socket_id(dstPort));
    // if (txBuffer == NULL)
    //     rte_exit(EXIT_FAILURE, "Cannot allocate buffer for tx on port %u\n",
    //              dstPort);

    uint32_t portMask = 0;
    portMask |= (1 << srcPort);
    portMask |= (1 << dstPort);
    // check_all_ports_link_status(portMask);
}

int sharedFd;
void SetupIPC()
{
    if (needMap == 0)
    {
        // truncate the file so that there won't be redundant content across multiple runs
        sharedFd = open("/dev/shm/nptn_shared_flag", O_RDWR | O_CREAT | O_TRUNC);
    }
    else
    {
        sharedFd = open("/dev/shm/nptn_shared_flag", O_RDONLY);
    }
    if (sharedFd == -1)
    {
        fprintf(stderr, "[SetupIPC] cannot open shared file\n");
        abort();
    }
}

uint16_t RxBurstNop(void *rxq, struct rte_mbuf **rx_pkts, uint16_t nb_pkts)
{
    return 0;
}

uint16_t TxBurstNop(void *txq, struct rte_mbuf **tx_pkts, uint16_t nb_pkts)
{
    return nb_pkts;
}

// use global eth devices to avoid malloc
static struct rte_eth_dev_data __space_dev_data;
static void *__space_rx_queues[2][16];
static void *__space_tx_queues[2][16];

void RedirectEthDevices(struct rte_eth_dev *devices)
{
    printf("setup eth devices at: %p\n", devices);
    for (int i = 0; i < 2; i += 1)
    {
        struct rte_eth_dev *dev = &devices[i];
        // dev->data = malloc(sizeof(struct rte_eth_dev_data));
        // dev->data->rx_queues = malloc(sizeof(void *) * NumberQueue);
        // dev->data->tx_queues = malloc(sizeof(void *) * NumberQueue);
        dev->data = &__space_dev_data;
        dev->data->rx_queues = __space_rx_queues[i];
        dev->data->tx_queues = __space_tx_queues[i];
    }
    devices[0].rx_pkt_burst = RxBurst;
    // devices[0].tx_pkt_burst = TxBurstNop;
    devices[0].tx_pkt_burst = TxBurst;
    devices[1].rx_pkt_burst = RxBurstNop;
    devices[1].tx_pkt_burst = TxBurst;
}