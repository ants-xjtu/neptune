#include "RunMoon.h"

struct MoonData
{
    //
};

int runtimePkey;

int main(int argc, char *argv[], char *envp[])
{
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");
    argc -= ret;
    argv += ret;

    SetupDpdk();

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
    interface->pcapLoop = PcapLoop;
    printf("HeapMalloc at address %p\n", HeapMalloc);
    printf("configure preloading for tiangou\n");
    PreloadLibrary(tiangou);
    printf("%s\n", DONE_STRING);

    for (int i = 2; i < argc; i++)
    {
        // we give each moon an id here, and it needs to be recycled later
        printf("loading moon%d...\n", i - 1);
        LoadMoon(argv[i], i - 2);
    }

    runtimePkey = pkey_alloc(0, 0);
    printf("Allocate pkey #%d for runtime\n", runtimePkey);
    // dummy page for being protected
    long pagesize = sysconf(_SC_PAGESIZE);
    unsigned char *dummy = aligned_alloc(pagesize, pagesize);
    ret = pkey_mprotect(dummy, pagesize, PROT_WRITE, runtimePkey);
    if (ret == -1)
    {
        rte_exit(EXIT_FAILURE, "pkey_mprotect: %d", ret);
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

void LoadMoon(char *moonPath, int MOON_ID)
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
    // interface->packetRegionLow = LibraryFind(&library, "SwordHolder_ExtraLow");
    // interface->packetRegionHigh = LibraryFind(&library, "SwordHolder_ExtraHigh");

    nf_state[MOON_ID] = LibraryFind(&library, "state");

    moonStart = LibraryFind(&library, "main");
    printf("entering MOON for initial running, start = %p\n", moonStart);
    StackStart(MOON_ID, initMoon);
    printf("%s\n", DONE_STRING);
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
        nb_rx = rte_eth_rx_burst(portid, 0, packetBurst, MAX_PKT_BURST);

        port_statistics.rx += nb_rx;
        burstSize = nb_rx;
        pkt_interval += nb_rx;

        // prevent unnecessary pkey overhead
        // IMPORTANT: careful when change code below here
        if (nb_rx == 0)
        {
            continue;
        }

        int status = pkey_set(runtimePkey, PKEY_DISABLE_WRITE);
        // if (status)
        // {
        //     fprintf(stderr, "pkey_set: %d\n", status);
        // }
        for (int i = 0; i < MoonNum; i++)
        {
            HeapSwitch(i);
            // TODO: switch interface->packetRegion*
            StackSwitch(i);
        }
        status = pkey_set(runtimePkey, 0);
        // if (status)
        // {
        //     fprintf(stderr, "pkey_set: %d\n", status);
        // }

        for (i = 0; i < nb_rx; i++)
        {
            struct rte_mbuf *m = packetBurst[i];
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

int PcapLoop(pcap_t *p, int cnt, pcap_handler callback, u_char *user)
{
    for (;;)
    {
        StackSwitch(-1);
        for (int i = 0; i < burstSize; i += 1)
        {
            rte_prefetch0(rte_pktmbuf_mtod(packetBurst[i], void *));
            struct pcap_pkthdr header;
            header.len = rte_pktmbuf_pkt_len(packetBurst[i]);
            header.caplen = rte_pktmbuf_data_len(packetBurst[i]);
            memset(&header.ts, 0x0, sizeof(header.ts)); // todo
            uintptr_t *packet = rte_pktmbuf_mtod(packetBurst[i], uintptr_t *);
            // *interface.packetRegionLow = (uintptr_t)packet;
            // *interface.packetRegionHigh = *interface.packetRegionLow + header.caplen;
            callback(user, &header, (u_char *)packet);
        }
    }
}
