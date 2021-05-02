#include "RunMoon.h"

void signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM)
    {
        printf("\nSignal %d received, preparing to exit...\n",
               signum);
        force_quit = true;
    }
}

/* Check the link status of all ports in up to 9s, and print them finally */
void check_all_ports_link_status(uint32_t port_mask)
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
            printf("\n%s\n", DONE_STRING);
        }
    }
}

void SetupDpdk()
{
    int ret;

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
}

static const size_t NumberQueue = 16;

void SetupEthDevices(struct rte_eth_dev *devices)
{
    printf("setup eth devices at: %p\n", devices);
    for (int i = 0; i < 2; i += 1)
    {
        struct rte_eth_dev *dev = &devices[i];
        printf("setup device: %p\n", dev);
        dev->rx_pkt_burst = RxBurst;
        dev->data = malloc(sizeof(struct rte_eth_dev_data));
        dev->data->rx_queues = malloc(sizeof(void *) * NumberQueue);
    }
}