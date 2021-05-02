#include "RunMoon/Common.h"

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
        printf("usage: %s [EAL options] -- [--pku] <tiangou> <moon> [<moons>]\n", argv[0]);
        return 0;
    }
    InitLoader(argc, argv, envp);

    const char *tiangouPath = argv[1];
    printf("loading tiangou library %s\n", tiangouPath);
    void *tiangou = dlopen(tiangouPath, RTLD_LAZY);
    interface = dlsym(tiangou, "interface");
    printf("injecting interface functions at %p\n", interface);
    interface->malloc = HeapMalloc;
    interface->realloc = HeapRealloc;
    interface->calloc = HeapCalloc;
    interface->free = HeapFree;
    interface->signal = signal;
    interface->pcapLoop = PcapLoop;
    struct rte_eth_dev *rteEthDevices = dlsym(tiangou, "rte_eth_devices");
    RedirectEthDevices(rteEthDevices);
    printf("configure preloading for tiangou\n");
    PreloadLibrary(tiangou);
    printf("%s\n", DONE_STRING);

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
        printf("[RunMoon] moon#%d START\n", moonId);
        LoadMoon(argv[i], moonId);
    }
    loading = 0;

    // according to pkey(7), "The default key is assigned to any memory region
    // for which a pkey has not been explicitly assigned via pkey_mprotect(2)."
    // However, pkey_set(0, ...) seems do not have any affect
    // so use a non-default key to protect runtime for now
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
    // int status = pkey_set(runtimePkey, PKEY_DISABLE_WRITE);
    // dummy[42] = 42;

    printf("*** START RUNTIME MAIN LOOP ***\n");
    MainLoop();

    return 0;
}

void InitMoon()
{
    char *argv[0];
    printf("[InitMoon] calling moonStart\n");
    moonStart(0, argv);
    printf("[InitMoon] nf exited before blocking on packets, intended?\n");
    exit(0);
}

void LoadMoon(char *moonPath, int moonId)
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
    SetStack(moonId, stackStart, STACK_SIZE);
    library.loadAddress = libraryStart;
    DeployLibrary(&library);
    SetHeap(heapStart, heapSize, moonId);
    InitHeap();
    printf("%s\n", DONE_STRING);

    printf("setting protection region for MOON\n");
    uintptr_t *mainPrefix = LibraryFind(&library, "SwordHolder_MainPrefix");
    *mainPrefix = (uintptr_t)arena;
    printf("main region prefix:\t%#lx (align 32GB)\n", *mainPrefix);

    moonStart = LibraryFind(&library, "main");
    printf("entering MOON for initial running, start = %p\n", moonStart);
    StackStart(moonId, InitMoon);
    printf("%s\n", DONE_STRING);

    printf("finalize moon setup\n");
    moonDataList[moonId].id = moonId;
    moonDataList[moonId + 1].id = -1;
    moonDataList[moonId].extraLowPtr = LibraryFind(&library, "SwordHolder_ExtraLow");
    moonDataList[moonId].extraHighPtr = LibraryFind(&library, "SwordHolder_ExtraHigh");
    if (!moonDataList[moonId].extraLowPtr || !moonDataList[moonId].extraHighPtr)
    {
        rte_exit(EXIT_FAILURE, "cannot resolve extra region variables");
    }
    moonDataList[moonId].pkey = pkey_alloc(0, 0);
    moonDataList[moonId].switchTo = -1;
    if (moonId != 0)
    {
        moonDataList[moonId - 1].switchTo = moonId;
    }
    // pkey_mprotect(arena, MOON_SIZE, PROT_READ | PROT_WRITE, moonDataList[moonId].pkey);
    printf("%s\n", DONE_STRING);
}

// https://code.woboq.org/userspace/glibc/sysdeps/unix/sysv/linux/x86/arch-pkey.h.html
/* Return the value of the PKRU register.  */
static inline unsigned int
pkey_read(void)
{
    unsigned int result;
    __asm__ volatile(".byte 0x0f, 0x01, 0xee"
                     : "=a"(result)
                     : "c"(0)
                     : "rdx");
    return result;
}

/* Overwrite the PKRU register with VALUE.  */
static inline void
pkey_write(unsigned int value)
{
    __asm__ volatile(".byte 0x0f, 0x01, 0xef"
                     :
                     : "a"(value), "c"(0), "d"(0));
}

unsigned int pkru;
int pkey_set(int key, unsigned int rights)
{
    if (key < 0 || key > 15 || rights > 3)
    {
        // __set_errno(EINVAL);
        return -1;
    }
    unsigned int mask = 3 << (2 * key);
    // unsigned int pkru = pkey_read();
    pkru = (pkru & ~mask) | (rights << (2 * key));
    // pkey_write(pkru);
    return 0;
}

void UpdatePkey()
{
    if (!enablePku)
    {
        return;
    }
    pkru = pkey_read();
    pkey_set(runtimePkey, PKEY_DISABLE_WRITE);
    for (int i = 0; moonDataList[i].id != -1; i += 1)
    {
        pkey_set(
            moonDataList[i].pkey,
            moonDataList[i].id == currentMoonId ? 0 : PKEY_DISABLE_WRITE);
    }
    pkey_write(pkru);
}

void DisablePkey()
{
    if (!enablePku)
    {
        return;
    }
    pkru = pkey_read();
    pkey_set(runtimePkey, 0);
    for (int i = 0; moonDataList[i].id != -1; i += 1)
    {
        pkey_set(moonDataList[i].pkey, 0);
    }
    pkey_write(pkru);
}

void MoonSwitch()
{
    if (currentMoonId != -1)
    {
        HeapSwitch(currentMoonId);
        UpdatePkey();
    }
    StackSwitch(currentMoonId); // maybe -1
}

#define BURST_TX_DRAIN_US 100 /* TX drain every ~100us */
#define MEMPOOL_CACHE_SIZE 256
#define RTE_LOGTYPE_L2FWD RTE_LOGTYPE_USER1

static uint64_t timer_period = 1; /* default period is 10 seconds */

void MainLoop(void)
{
    int sent;
    unsigned lcore_id;
    uint64_t prev_tsc, diff_tsc, cur_tsc, timer_tsc;
    unsigned i, j, portid, nb_rx;
    const uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S *
                               BURST_TX_DRAIN_US;
    struct rte_eth_dev_tx_buffer *buffer;

    prev_tsc = 0;
    timer_tsc = 0;
    lcore_id = rte_lcore_id();
    uint64_t numberTimerSecond = timer_period;
    timer_period *= rte_get_timer_hz();

    RTE_LOG(INFO, L2FWD, "entering main loop on lcore %u\n", lcore_id);

#define CYCLE_SIZE 99
    double cycle[CYCLE_SIZE], prevAvg = -1;
    int cycleIndex = 0;
#define AVG_DELTA_CYCLE_SIZE 24
    int avgDeltaCycle[AVG_DELTA_CYCLE_SIZE];
    int avgDeltaCycleIndex = 0;

    while (!force_quit)
    {
        cur_tsc = rte_rdtsc();

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
                    /* reset the timer */
                    timer_tsc = 0;
                    double pps = (double)port_statistics.tx / numberTimerSecond / 1000;
                    printf("pps: %.3fK", pps);
                    memset(&port_statistics, 0, sizeof(port_statistics));

                    cycle[cycleIndex % CYCLE_SIZE] = pps;
                    if (pps != 0.0 && !(prevAvg >= 0.0 && pps < 0.8 * prevAvg))
                    {
                        cycleIndex += 1;
                        printf("        ");
                    }
                    else
                    {
                        printf(" (ignored)");
                    }
                    printf("\t");
                    double sum = 0.0;
                    int count = cycleIndex >= CYCLE_SIZE ? CYCLE_SIZE : cycleIndex;
                    for (int i = 0; i < count; i += 1)
                    {
                        sum += cycle[i];
                    }
                    double avg = sum / count;
                    printf("avg(over past %2d): %fK\t", count, avg);

                    avgDeltaCycle[avgDeltaCycleIndex % AVG_DELTA_CYCLE_SIZE] = prevAvg < avg;
                    avgDeltaCycleIndex += 1;
                    int go[] = {0, 0};
                    for (int i = 0; i < avgDeltaCycleIndex; i += 1)
                    {
                        if (i >= AVG_DELTA_CYCLE_SIZE)
                        {
                            break;
                        }
                        go[avgDeltaCycle[i]] += 1;
                    }
                    printf(
                        "%2d go up, %2d go down in last %2d avg\n",
                        go[0], go[1],
                        avgDeltaCycleIndex > AVG_DELTA_CYCLE_SIZE ? AVG_DELTA_CYCLE_SIZE : avgDeltaCycleIndex);
                    prevAvg = avg;
                }
            }

            prev_tsc = cur_tsc;
        }

        /*
		 * Read packet from RX queues
		 */
        portid = srcPort;
        nb_rx = rte_eth_rx_burst(portid, 0, packetBurst, MAX_PKT_BURST);
        port_statistics.rx += nb_rx;
        burstSize = nb_rx;

        // prevent unnecessary pkey overhead
        // IMPORTANT: careful when change code below here
        if (nb_rx == 0)
        {
            continue;
        }

        currentMoonId = 0;
        MoonSwitch();
        DisablePkey();

        for (i = 0; i < nb_rx; i++)
        {
            struct rte_mbuf *m = packetBurst[i];
            sent = rte_eth_tx_buffer(dstPort, 0, txBuffer, m);
            if (sent)
                port_statistics.tx += sent;
        }
    }
    printf("Keep calm and definitely full-force fighting for SOSP.\n");
}

int PcapLoop(pcap_t *p, int cnt, pcap_handler callback, u_char *user)
{
    for (;;)
    {
        if (loading)
        {
            StackSwitch(-1);
        }
        else
        {
            currentMoonId = moonDataList[currentMoonId].switchTo;
            MoonSwitch();
        }

        for (int i = 0; i < burstSize; i += 1)
        {
            rte_prefetch0(rte_pktmbuf_mtod(packetBurst[i], void *));
            struct pcap_pkthdr header;
            header.len = rte_pktmbuf_pkt_len(packetBurst[i]);
            header.caplen = rte_pktmbuf_data_len(packetBurst[i]);
            memset(&header.ts, 0x0, sizeof(header.ts)); // todo
            uintptr_t *packet = rte_pktmbuf_mtod(packetBurst[i], uintptr_t *);
            // *moonDataList[currentMoonId].extraLowPtr = (uintptr_t)packet;
            // *moonDataList[currentMoonId].extraHighPtr = (uintptr_t)packet + header.caplen;
            callback(user, &header, (u_char *)packet);
        }
    }
}

uint16_t RxBurst(void *rxq, struct rte_mbuf **rx_pkts, uint16_t nb_pkts)
{
    printf("[RunMoon] RxBurst: sucess\n");
    exit(0);
}