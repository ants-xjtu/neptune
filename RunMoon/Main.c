#include "Common.h"

uint64_t timer_period = 1; /* default period is 10 seconds */

int main(int argc, char *argv[])
{
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");
    argc -= ret;
    argv += ret;

    SetupDpdk();
    numberTimerSecond = timer_period;
    timer_period *= rte_get_timer_hz();

    if (argc < 3)
    {
        printf("usage: %s [EAL options] -- [--pku] <tiangou> <moon> [<moons>]\n", argv[0]);
        return 0;
    }
    InitLoader(argc, argv, environ);

    const char *tiangouPath = argv[1];
    printf("loading tiangou library %s\n", tiangouPath);
    void *tiangou = dlopen(tiangouPath, RTLD_LAZY);
    interface = dlsym(tiangou, "interface");
    printf("injecting TIANGOU interface at %p\n", interface);
    interface->malloc = HeapMalloc;
    interface->realloc = HeapRealloc;
    interface->calloc = HeapCalloc;
    interface->free = HeapFree;
    interface->signal = signal;
    interface->pcapLoop = PcapLoop;
    struct rte_eth_dev_info srcInfo, dstInfo;
    rte_eth_dev_info_get(srcPort, &srcInfo);
    rte_eth_dev_info_get(dstPort, &dstInfo);
    interface->srcInfo = &srcInfo;
    interface->dstInfo = &dstInfo;
    interface->tscHz = rte_get_tsc_hz();

    struct rte_eth_dev *rteEthDevices = dlsym(tiangou, "rte_eth_devices");
    RedirectEthDevices(rteEthDevices);

    printf("configure preloading for tiangou\n");
    PreloadLibrary(tiangou);
    printf("%s: tiangou\n", DONE_STRING);

    int i = 2;
    enablePku = 0;
    if (strcmp(argv[2], "--pku") == 0)
    {
        i += 1;
        enablePku = 1;
    }

    loading = 1;
    for (int moonId = 0; i < argc; moonId += 1, i += 1)
    {
        char *moonPath = argv[i];
        printf("[RunMoon] moon#%d START LOADING\n", moonId);
        LoadMoon(argv[i], moonId);
    }
    loading = 0;

    // according to pkey(7), "The default key is assigned to any memory region
    // for which a pkey has not been explicitly assigned via pkey_mprotect(2)."
    // However, pkey_set(0, ...) seems do not have any effect
    // so use a non-default key to protect runtime for now
    runtimePkey = pkey_alloc(0, 0);
    printf("Allocate pkey %%%d for runtime\n", runtimePkey);
    // dummy page for being protected
    long pagesize = sysconf(_SC_PAGESIZE);
    unsigned char *dummy = aligned_alloc(pagesize, pagesize);
    ret = pkey_mprotect(dummy, pagesize, PROT_WRITE, runtimePkey);
    if (ret == -1)
    {
        rte_exit(EXIT_FAILURE, "pkey_mprotect: %d", ret);
    }

    printf("assign rx/tx queues for workers\n");
    int rx = 0, tx = 0;
    unsigned int workerId;
    RTE_LCORE_FOREACH_WORKER(workerId)
    {
        workerDataList[workerId].rxQueue = rx;
        workerDataList[workerId].txQueue = tx;
        rx += 1;
        tx += 1;
    }

    printf("[RunMoon] launch workers\n");
    RTE_LCORE_FOREACH_WORKER(workerId)
    {
        rte_eal_remote_launch(MainLoop, NULL, workerId);
    }

    printf("*** START RUNTIME MAIN LOOP ***\n");
    while (!force_quit)
    {
        PrintBench();
        // TODO: supervisor tasks, update chain, redirect traffic, etc.
    }

    RTE_LCORE_FOREACH_WORKER(workerId)
    {
        rte_eal_wait_lcore(workerId);
    }

    printf("Keep calm and definitely full-force fighting for SOSP.\n");
    return 0;
}

void LoadMoon(char *moonPath, int moonId)
{
    printf("[LoadMoon] MOON#%d global registration\n", moonId);
    moonDataList[moonId].id = moonId;
    moonDataList[moonId + 1].id = -1;
    moonDataList[moonId].pkey = pkey_alloc(0, 0);
    printf("moon pkey %%%d\n", moonDataList[moonId].pkey);
    moonDataList[moonId].switchTo = -1;
    if (moonId != 0)
    {
        moonDataList[moonId - 1].switchTo = moonId;
    }

    unsigned int workerId;
    RTE_LCORE_FOREACH_WORKER(workerId)
    {
        int instanceId = (workerId << 4) | (unsigned)moonId;
        printf("[LoadMoon] MOON#%d @ worker$%d (inst!%03x)\n", moonId, workerId, instanceId);
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
        // printf("\n");

        printf("initializing regions\n");
        SetStack(instanceId, stackStart, STACK_SIZE);
        library.loadAddress = libraryStart;
        DeployLibrary(&library);
        SetHeap(heapStart, heapSize, instanceId);
        InitHeap();
        printf("%s: initialized\n", DONE_STRING);

        printf("setting protection region for MOON\n");
        uintptr_t *mainPrefix = LibraryFind(&library, "SwordHolder_MainPrefix");
        *mainPrefix = (uintptr_t)arena;
        printf("main region prefix:\t%#lx (align 32GB)\n", *mainPrefix);
        // protecting the stack will cause the calling `StackSwitch` or `UpdatePkey` itself to fail
        // there must be a way to deal with it...
        // pkey_mprotect(arena, MOON_SIZE, PROT_READ | PROT_WRITE, moonDataList[moonId].pkey);
        pkey_mprotect(arena + STACK_SIZE, MOON_SIZE - STACK_SIZE, PROT_READ | PROT_WRITE, moonDataList[moonId].pkey);

        // need this to print corrent address so the value can be modified
        LibraryFind(&library, "per_lcore__lcore_id");

        printf("register MOON#%d worker$%d data (inst!%03x)\n", moonId, workerId, instanceId);
        moonDataList[moonId].workers[workerId].instanceId = instanceId;
        uintptr_t *extraLow = LibraryFind(&library, "SwordHolder_ExtraLow"),
                  *extraHigh = LibraryFind(&library, "SwordHolder_ExtraHigh");
        if (!extraLow || !extraHigh)
        {
            rte_exit(EXIT_FAILURE, "cannot resolve extra region variables");
        }
        moonDataList[moonId].workers[workerId].extraLowPtr = extraLow;
        moonDataList[moonId].workers[workerId].extraHighPtr = extraHigh;
        printf("%s: register MOON#%d worker$%d\n", DONE_STRING, moonId, workerId);

        moonStart = LibraryFind(&library, "main");
        printf("entering MOON for initial running, start = %p\n", moonStart);
        isDpdkMoon = 0;
        StackStart(instanceId, InitMoon);
        printf("%s: MOON initialization, isDpdk = %d\n", DONE_STRING, isDpdkMoon);
        if (isDpdkMoon)
        {
            *extraLow = mbufLow;
            *extraHigh = mbufHigh;
            printf("one-time setting extra region for DPDK: %#lx - %#lx\n", *extraLow, *extraHigh);
        }
    }
}

void MoonSwitch(unsigned int workerId)
{
    if (workerDataList[workerId].current != -1) // not switching back to runtime
    {
        int instanceId =
            moonDataList[workerDataList[workerId].current]
                .workers[workerId]
                .instanceId;
        HeapSwitch(instanceId);
        UpdatePkey(workerId);
        StackSwitch(instanceId);
    }
    else
    {
        StackSwitch(-1);
    }
}

#define BURST_TX_DRAIN_US 100 /* TX drain every ~100us */
#define MEMPOOL_CACHE_SIZE 256
#define RTE_LOGTYPE_L2FWD RTE_LOGTYPE_USER1

int MainLoop(void *_arg)
{
    int sent;
    unsigned lcore_id, workerId;
    uint64_t prev_tsc, diff_tsc, cur_tsc, timer_tsc;
    unsigned i, j, portid, nb_rx;
    const uint64_t drain_tsc =
        (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S * BURST_TX_DRAIN_US;
    struct rte_eth_dev_tx_buffer *buffer;

    prev_tsc = 0;
    timer_tsc = 0;
    workerId = lcore_id = rte_lcore_id();

    DisablePkey(1);
    RTE_LOG(INFO, L2FWD, "entering main loop on lcore $%u\n", lcore_id);

    while (!force_quit)
    {
        cur_tsc = rte_rdtsc();
        /*
		 * TX burst queue drain
		 */
        diff_tsc = cur_tsc - prev_tsc;
        if (unlikely(diff_tsc > drain_tsc))
        {
            // portid = dstPort;
            // buffer = txBuffer;
            // sent = rte_eth_tx_buffer_flush(portid, workerDataList[workerId].txQueue, buffer);
            // if (sent)
            //     workerDataList[workerId].stat.tx += sent;

            /* if timer is enabled */
            if (timer_period > 0)
            {
                /* advance the timer */
                timer_tsc += diff_tsc;
                /* if timer has reached its timeout */
                if (unlikely(timer_tsc >= timer_period))
                {
                    /* reset the timer */
                    timer_tsc = 0;
                    RecordBench(cur_tsc);
                }
            }
            prev_tsc = cur_tsc;
        }

        /*
		 * Read packet from RX queues
		 */
        portid = srcPort;
        nb_rx = rte_eth_rx_burst(
            portid, workerDataList[workerId].rxQueue, workerDataList[workerId].packetBurst, MAX_PKT_BURST);
        workerDataList[workerId].stat.rx += nb_rx;
        workerDataList[workerId].burstSize = nb_rx;

        // prevent unnecessary pkey overhead
        // IMPORTANT: careful when change code below this
        if (nb_rx == 0)
        {
            continue;
        }

        workerDataList[workerId].current = 0;
        MoonSwitch(workerId);
        DisablePkey(0);

        // for (i = 0; i < nb_rx; i++)
        // {
        //     struct rte_mbuf *m = workerDataList[workerId].packetBurst[i];
        //     sent = rte_eth_tx_buffer(dstPort, workerDataList[workerId].txQueue, txBuffer, m);
        //     if (sent)
        //         workerDataList[workerId].stat.tx += sent;
        // }
        sent = rte_eth_tx_burst(
            dstPort, workerDataList[workerId].txQueue,
            workerDataList[workerId].packetBurst, workerDataList[workerId].burstSize);
        if (sent)
            workerDataList[workerId].stat.tx += sent;
    }
    printf("[RunMoon] worker on lcore$%d exit\n", lcore_id);
    return 0;
}

// the following functions are executed in MOON env
// i.e. on private stack with private heap
void InitMoon()
{
    char *argv[0];
    printf("[InitMoon] calling moonStart\n");
    moonStart(0, argv);
    printf("[InitMoon] nf exited before blocking on packets, intended?\n");
    exit(0);
}

int PcapLoop(pcap_t *p, int cnt, pcap_handler callback, u_char *user)
{
    unsigned int workerId;
    for (;;)
    {
        if (loading)
        {
            StackSwitch(-1);
            workerId = rte_lcore_id();
        }
        else
        {
            workerDataList[workerId].current = moonDataList[workerDataList[workerId].current].switchTo;
            MoonSwitch(workerId);
        }

        for (int i = 0; i < workerDataList[workerId].burstSize; i += 1)
        {
            rte_prefetch0(rte_pktmbuf_mtod(workerDataList[workerId].packetBurst[i], void *));
            struct pcap_pkthdr header;
            header.len = rte_pktmbuf_pkt_len(workerDataList[workerId].packetBurst[i]);
            header.caplen = rte_pktmbuf_data_len(workerDataList[workerId].packetBurst[i]);
            memset(&header.ts, 0x0, sizeof(header.ts)); // todo
            uintptr_t *packet = rte_pktmbuf_mtod(workerDataList[workerId].packetBurst[i], uintptr_t *);
            *moonDataList[workerDataList[workerId].current].workers[workerId].extraLowPtr = (uintptr_t)packet;
            *moonDataList[workerDataList[workerId].current].workers[workerId].extraHighPtr = (uintptr_t)packet + header.caplen;
            callback(user, &header, (u_char *)packet);
        }
    }
}

uint16_t RxBurst(void *rxq, struct rte_mbuf **rx_pkts, uint16_t nb_pkts)
{
    // printf("[RunMoon] RxBurst: sucess\n");
    // exit(0);

    unsigned int workerId = rte_lcore_id();
    if (loading)
    {
        isDpdkMoon = 1;
        StackSwitch(-1);
    }
    else
    {
        workerDataList[workerId].current = moonDataList[workerDataList[workerId].current].switchTo;
        // printf("switch to %d\n", workerDataList[workerId].current);
        MoonSwitch(workerId);
    }

    if (nb_pkts < workerDataList[workerId].burstSize)
    {
        fprintf(stderr, "MOON rx burst size too small: %u\n", nb_pkts);
        abort();
    }
    uint16_t size = workerDataList[workerId].burstSize;
    memcpy(rx_pkts, workerDataList[workerId].packetBurst, size * sizeof(struct rte_mbuf *));
    // clear burst
    workerDataList[workerId].burstSize = 0;
    return size;
}

uint16_t TxBurst(void *txq, struct rte_mbuf **tx_pkts, uint16_t nb_pkts)
{
    unsigned int workerId = rte_lcore_id();
    if (workerDataList[workerId].burstSize + nb_pkts > MAX_PKT_BURST)
    {
        fprintf(stderr, "MOON tx burst size too large\n");
        abort();
    }
    memcpy(
        &workerDataList[workerId].packetBurst[workerDataList[workerId].burstSize],
        tx_pkts, nb_pkts * sizeof(struct rte_mbuf *));
    workerDataList[workerId].burstSize += nb_pkts;
    return nb_pkts;
}