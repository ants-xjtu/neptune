#include "Common.h"
#include "../Loader/NFlink.h"

uint64_t timer_period = 1; /* default period is 10 seconds */

struct MoonConfig
{
    char *path;
    char *argv[10];
    int argc;
};
static struct MoonConfig CONFIG[] = {
    {.path = "./libs/libMoon_Libnids.so", .argv = {}, .argc = 0},
    {.path = "./libs/libMoon_prads.so", .argv = {}, .argc = 0},
    {.path = "./libs/L2Fwd/libMoon_L2Fwd.so", .argv = {"<program>", "-p", "0x3", "-q", "2"}, .argc = 5},
    {.path = "./libs/fastclick/click", .argv = {"<program>", "--dpdk", "-c", "0x1", "--", "/home/hypermoon/neptune-yh/dpdk-bounce.click"}, .argc = 6},
    // {.path = "./libs/fastclick/click", .argv = {"<program>", "--dpdk", "-c", "0x1", "--", "/home/hypermoon/neptune-yh/dpdk-bounce-heavy.click"}, .argc = 6},
    {.path = "./libs/Libnids/forward.so", .argv = {}, .argc = 0},
    {.path = "./libs/ndpi-new/ndpiReader.so", .argv = {"<program>", "-i", "ens3f0"}, .argc = 3},
    // #6: this config might not be used to directly call main
    {.path = "./libs/NetBricks/libzcsi_lpm.so", .argv = {"<program>", "-c", "1", "-p", "06:00.0"}, .argc = 5},
    // {.path = "./libs/rubik/rubik.so", .argv = {"<program>", "-p", "0x1"}, .argc = 3},
    {.path = "./libs/rubik-new/rubik.so", .argv = {"<program>", "-p", "0x1"}, .argc = 3},
    // {.path = "./libs/rubik/rubik.so", .argv = {"<program>", "-p", "0x3", "-q", "2"}, .argc = 5},
    // {.path = "./libs/Libnids-slow/libMoon_Libnids_Slow.so", .argv = {}, .argc = 0},
    {.path = "./nfd/five/rtc.so", .argv = {"<program>"}, .argc = 1},
    {.path = "./nfd/napt/napt", .argv = {"<program>"}, .argc = 1},
    {.path = "./nfd/hhd/hhd", .argv = {"<program>"}, .argc = 1},
    {.path = "./nfd/firewall/firewall", .argv = {"<program>"}, .argc = 1},
    {.path = "./nfd/ssd/ssd", .argv = {"<program>"}, .argc = 1},
    {.path = "./nfd/udpfm/udpfm", .argv = {"<program>"}, .argc = 1},
    {.path = "./onvm/scan/scan", .argv = {"<program>"}, .argc = 1},
    {.path = "./onvm/firewall/firewall", .argv = {"<program>"}, .argc = 1},
    {.path = "./onvm/encrypt/encrypt", .argv = {"<program>"}, .argc = 1},
    {.path = "./onvm/decrypt/decrypt", .argv = {"<program>"}, .argc = 1},
};

// static const char *CONFIG[][2] = {
//     { ""},
//     {"./build/libMoon_MidStat_NoSFI.so", ""},
//     {"./build/libMoon_MidStat_NoSFI.so", ""},
// };

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
    struct rte_eth_dev_info srcInfo, dstInfo;
    rte_eth_dev_info_get(srcPort, &srcInfo);
    rte_eth_dev_info_get(dstPort, &dstInfo);

    if (argc < 3)
    {
        printf("usage: %s [EAL options] -- [--pku] <tiangou> <moon_idx> [<moon_idxs>]\n", argv[0]);
        return 0;
    }
    // InitLoader(argc, argv, environ);

    const char *tiangouPath = argv[1];
    printf("loading tiangou library %s\n", tiangouPath);
    void *tiangou = dlopen(tiangouPath, RTLD_LAZY);
    Interface *interfacePointer = dlsym(tiangou, "interface");
    printf("injecting TIANGOU interface at %p\n", interfacePointer);
    interfacePointer->malloc = HeapMalloc;
    interfacePointer->realloc = HeapRealloc;
    interfacePointer->calloc = HeapCalloc;
    interfacePointer->alignedAlloc = HeapAlignedAlloc;
    interfacePointer->free = HeapFree;
    interfacePointer->signal = signal;
    interfacePointer->sigaction = sigaction;
    interfacePointer->pcapLoop = PcapLoop;
    interfacePointer->pcapNext = PcapNext;
    interfacePointer->pcapDispatch = PcapDispatch;
    interfacePointer->srcInfo = &srcInfo;
    interfacePointer->dstInfo = &dstInfo;
    interfacePointer->tscHz = rte_get_tsc_hz();
    interfacePointer->pthreadCreate = PthreadCreate;
    interfacePointer->pthreadCondTimedwait = PthreadCondTimedwait;
    interfacePointer->pthreadCondWait = PthreadCondWait;
    interfacePointer->lpm_create = rte_lpm_create;
    interfacePointer->lpm_find_existing = rte_lpm_find_existing;
    interfacePointer->lpm_add = rte_lpm_add;
    printf("configure preloading for tiangou\n");
    // no need for we are hard-coding tiangou in binaries
    // PreloadLibrary(tiangou);
    printf("%s: tiangou\n", DONE_STRING);

    int i = 2;
    enablePku = 0;
    if (strcmp(argv[2], "--pku") == 0)
    {
        i += 1;
        enablePku = 1;
    }

    int configIndex[16];
    for (int moonId = 0; i < argc; moonId += 1, i += 1)
    {
        configIndex[moonId] = atoi(argv[i]);
        configIndex[moonId + 1] = -1;
    }
    loading.inProgress = 1;
    for (int moonId = 0; configIndex[moonId] != -1; moonId += 1)
    {
        char *moonPath = CONFIG[configIndex[moonId]].path;
        printf("START LOADING moon#%d\n", moonId);
        LoadMoon(moonPath, moonId, configIndex[moonId]);
    }
    loading.inProgress = 0;

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

    printf("Keep calm and definitely full-force fighting for NSDI.\n");
    return 0;
}

static void RewriteMoonPath(struct PrivateLibrary *library, int workerId, int moonId)
{
    // using renaming to fix multi-instance Moon. Works for multi-core and 
    // uni-core duplicated instances.
    int instanceCtr = 0;
    for (int i = 0; i < moonId; i++)
    {
        if (moonDataList[i].config == moonDataList[moonId].config)
            instanceCtr++;
    }
    if (workerId == 1 && instanceCtr == 0)
        // lcore 1 is handled by default
        return;
    // NB: simply do not duplicate instance in MC
    int newId;
    if (workerId == 1)
    {
        // if we are duplicating instances, assume we are on worker$1
        newId = instanceCtr + 1;
    }
    else
    {
        // otherwise, the index will increase with the number of worker count
        newId = loading.workerCount + 1;
    }
    char workerid[16];
    sprintf(workerid, "%d", newId);
    char *p = malloc(strlen(library->file) + 5);
    strcpy(p, library->file);
    // reversely find the last '/'
    int pos;
    for (pos = strlen(library->file);; pos--)
    {
        if (*(library->file + pos) == '/')
            break;
    }
    strcpy(p + pos, workerid);
    strcat(p, library->file + pos);
    library->file = p;
    printf("[RewriteMoonPath] moon path is now at: %s\n", p);
}

void LoadMoon(char *moonPath, int moonId, int configIndex)
{
    printf("[LoadMoon] MOON#%d global registration\n", moonId);
    moonDataList[moonId].id = moonId;
    moonDataList[moonId].config = configIndex;
    moonDataList[moonId + 1].id = -1;
    moonDataList[moonId].pkey = pkey_alloc(0, 0);
    printf("moon pkey %%%d\n", moonDataList[moonId].pkey);
    moonDataList[moonId].switchTo = -1;
    if (moonId != 0)
    {
        moonDataList[moonId - 1].switchTo = moonId;
    }

    unsigned int workerId;
    loading.workerCount = 0;
    RTE_LCORE_FOREACH_WORKER(workerId)
    {
        int instanceId = (workerId << 4) | (unsigned)moonId;
        struct timeval loadStart, loadFinish;
        gettimeofday(&loadStart, NULL);
        printf("[LoadMoon] MOON#%d @ worker$%d (inst!%03x)\n", moonId, workerId, instanceId);
        printf("allocating memory for MOON %s\n", moonPath);
        // void *arena = aligned_alloc(MOON_SIZE, MOON_SIZE);
        void *arena = aligned_alloc(sysconf(_SC_PAGESIZE), MOON_SIZE);
        if (arena == NULL)
        {
            // if there are really a lot of NFs, this could fail
            printf("[LoadMoon] Not enough memory when allocating memory for MOON#%d @ worker$%d\n", moonId, workerId);
            exit(-1);
        }
        printf("arena address %p size %#lx\n", arena, MOON_SIZE);

        struct PrivateLibrary library;
        library.file = moonPath;
        RewriteMoonPath(&library, workerId, moonId);
        // we don't need to know the size of a library beforehand now
        // LoadLibrary(&library);
        // printf("library requires space: %#lx\n", library.length);
        void *stackStart = arena;
        void *heapStart = arena + NUMBER_STACK * STACK_SIZE;
        size_t heapSize = MOON_SIZE - NUMBER_STACK * STACK_SIZE;

        printf("*** MOON memory layout ***\n");
        printf("Stack#0\t%p - %p\tsize: %#lx\n", stackStart, stackStart + STACK_SIZE, STACK_SIZE);
        printf("Heap\t%p - %p\tsize: %#lx\n", heapStart, heapStart + heapSize, heapSize);
        // printf("\n");

        printf("initializing regions\n");
        SetStack(instanceId, stackStart, STACK_SIZE);

        SetHeap(heapStart, heapSize, instanceId);
        InitHeap();
        printf("%s: initialized\n", DONE_STRING);
        
        // I want dlopen to warn me if it is complaining
        if (DeployLibrary(&library))
        {
            fprintf(stderr, "loading %s as MOON failed, reason: %s\n", library.file, dlerror());
            exit(-1);
        }
        

        printf("setting protection region for MOON\n");
        // uintptr_t *mainPrefix = LibraryFind(&library, "SwordHolder_MainPrefix");
        // *mainPrefix = (uintptr_t)arena;
        // printf("main region prefix:\t%#lx (align 32GB)\n", *mainPrefix);
        // protecting the stack will cause the calling `StackSwitch` or `UpdatePkey` itself to fail
        // there must be a way to deal with it...
        // pkey_mprotect(arena, MOON_SIZE, PROT_READ | PROT_WRITE, moonDataList[moonId].pkey);

        // TODO: find a way to correctly protect the shared object's region
        pkey_mprotect(arena + STACK_SIZE, MOON_SIZE - STACK_SIZE, PROT_READ | PROT_WRITE, moonDataList[moonId].pkey);
        // ProtectMoon(library.file, moonDataList[moonId].pkey);

        // wierd thing here: `*core_id = 0` must after `pkey_mprotect`, or SEGFAULT
        // totally cannot understand
        printf("inject global variable for MOON library\n");
        // the semantics is a little different, but I believe the results will be the same
        // Originally NFsym only tells if the NF itself needs `lcore_id', but now even the NF does not need it
        // if the library NF depends define `lcore_id', it would be returned by dlsym.
        // But it's OK. The NF does not use it anyway
        unsigned *core_id = LibraryFind(&library, "per_lcore__lcore_id");
        printf("core_id at %p\n", core_id);
        if (core_id)
        {
            *core_id = 0;
        }
        struct rte_eth_dev *rteEthDevices = LibraryFind(&library, "rte_eth_devices");
        if (rteEthDevices)
        {
            RedirectEthDevices(rteEthDevices);
        }
        printf("%s\n", DONE_STRING);

        printf("register MOON#%d worker$%d data (inst!%03x)\n", moonId, workerId, instanceId);
        moonDataList[moonId].workers[workerId].instanceId = instanceId;
        // uintptr_t *extraLow = LibraryFind(&library, "SwordHolder_ExtraLow"),
        //           *extraHigh = LibraryFind(&library, "SwordHolder_ExtraHigh");
        // if (!extraLow || !extraHigh)
        // {
        //     rte_exit(EXIT_FAILURE, "cannot resolve extra region variables");
        // }
        // moonDataList[moonId].workers[workerId].extraLowPtr = extraLow;
        // moonDataList[moonId].workers[workerId].extraHighPtr = extraHigh;
        printf("%s: register MOON#%d worker$%d\n", DONE_STRING, moonId, workerId);

        loading.moonStart = LibraryFind(&library, "main");
        if (loading.moonStart == NULL)
        {
            // hard code for fast click now
            // this may cause problems, so print it verbosely
            printf("main function not found in %s! Is it intentional?\n", library.file);
            struct NF_link_map *l = library.loadAddress;
            loading.moonStart = (void *)(l->l_addr + 0x16db70);
        }
        gettimeofday(&loadFinish, NULL);
        double loadTime = (double)(loadFinish.tv_usec - loadStart.tv_usec) / 1000000 + loadFinish.tv_sec - loadStart.tv_sec;
        printf("MOON#%d on worker$%d finished loading, time elapsed: %fs\n", moonId, workerId, loadTime);
        printf("entering MOON for initial running, start = %p\n", loading.moonStart);
        loading.isDpdkMoon = 0;
        loading.instanceId = instanceId;
        loading.heapStart = heapStart;
        loading.heapSize = heapSize;
        loading.threadStackIndex = 0;
        loading.stackStart = stackStart;
        loading.configIndex = configIndex;
        loading.workerCount++;
        StackStart(instanceId, InitMoon);
        printf("%s: MOON initialization, isDpdk = %d\n", DONE_STRING, loading.isDpdkMoon);
        // if (loading.isDpdkMoon)
        // {
        //     *extraLow = mbufLow;
        //     *extraHigh = mbufHigh;
        //     printf("one-time setting extra region for DPDK: %#lx - %#lx\n", *extraLow, *extraHigh);
        // }
    }
}

uint64_t ss_clk0, ss_clk1, up_clk0, up_clk1;
uint64_t up_sum = 0;
// idx 0 is the last but one NF to last NF, and so on
// TODO: scale these variables up for multi-core
uint64_t sum_array[16];
int sum_idx = 0;
int benchCounter = 0;
int upCounter = 0;

int up_idx = 0;
uint64_t up_array[16];

int hsCounter = 0;
uint64_t hs_clk0, hs_clk1;
uint64_t hsSum = 0;

static void UpdatePkeyBench(unsigned int workerId)
{
    // update pkey, with overhead recorded
    up_clk0 = rte_rdtsc();
    UpdatePkey(workerId);
    up_clk1 = rte_rdtsc();
    // printf("hs_clk0: %" PRIu64 "\ths_clk1: %" PRIu64 " diff: %" PRIu64 "\n", up_clk0, up_clk1, up_clk1 - up_clk0);
    // up_sum += up_clk1 - up_clk0;
    up_array[up_idx++] += up_clk1 - up_clk0;
    // upCounter += 1;
}

static inline void StackSwitchBench(unsigned int workerId, unsigned int instanceId)
{
    // TODO2: making evaluating benchmark a compile option
    if (instanceId != -1)
    {
        ss_clk0 = rte_rdtsc();
        StackSwitch(instanceId);
        ss_clk1 = rte_rdtsc();
        sum_array[sum_idx++] += ss_clk1 - ss_clk0;
    }
    else
    {
        ss_clk0 = rte_rdtsc();
        StackSwitch(-1);
        ss_clk1 = rte_rdtsc();
        sum_idx = 0;
        sum_array[sum_idx++] += ss_clk1 - ss_clk0;
        benchCounter++;
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
        // hs_clk0 = rte_rdtsc();
        HeapSwitch(instanceId);
        // hs_clk1 = rte_rdtsc();
        UpdatePkey(workerId);
        // UpdatePkeyBench(workerId);
        // hs_clk1 = rte_rdtsc();
        // printf("hs_clk0: %" PRIu64 "\ths_clk1: %" PRIu64 "diff: %" PRIu64 "\n", hs_clk0, hs_clk1, hs_clk1 - hs_clk0);
        // hsSum += hs_clk1 - hs_clk0;
        // hsCounter++;
        StackSwitch(instanceId);
        // StackSwitchBench(workerId, instanceId);
    }
    else
    {
        StackSwitch(-1);
        // StackSwitchBench(workerId, -1);
    }
}

static void ssPrintBench()
{
    // TODO: move this function next to PrintBench and do not share the
    // timer with tx drain
    if (benchCounter)
    {
        for (int i = 0; sum_array[i] != 0; i++)
        {
            printf("overhead #%d: %fcycles\t", i, (double)sum_array[i]/benchCounter);
        }
        printf("\n");
        memset(sum_array, 0, sizeof(uint64_t) * 16);
        benchCounter = 0;
    }
}

static void upPrintBench()
{
    if (upCounter)
    {
        for (int i = 0; up_array[i] != 0; i++)
        {
            printf("overhead #%d: %fcycles\t", i, (double)up_array[i]/upCounter);
        }
        printf("\n");
        memset(up_array, 0, sizeof(uint64_t) * 16);
        upCounter = 0;
    }
}

static void hsPrintBench()
{
    if (hsCounter)
    {
        double overhead = (double)hsSum / hsCounter;
        printf("heapSwitch overhead: %f\n", overhead);
        hsCounter = 0;
        hsSum = 0;
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
    uint64_t postRx, preTx;
    prev_tsc = 0;
    timer_tsc = 0;
    workerId = lcore_id = rte_lcore_id();

    DisablePkey(1);
    RTE_LOG(INFO, L2FWD, "entering main loop on lcore $%u\n", lcore_id);
    printf("entering main loop on lcore $%u\n", lcore_id);
    printf("lcore %u is processing rx queue %d, tx queue %d\n", 
            workerId,
            workerDataList[workerId].rxQueue, 
            workerDataList[workerId].txQueue);
    while (!force_quit)
    {
        // cur_tsc = rte_rdtsc();
        /*
		 * TX burst queue drain
		 */
        // diff_tsc = cur_tsc - prev_tsc;
        // if (unlikely(diff_tsc > drain_tsc))
        // {
        //     /* if timer is enabled */
        //     if (timer_period > 0)
        //     {
        //         /* advance the timer */
        //         timer_tsc += diff_tsc;
        //         /* if timer has reached its timeout */
        //         if (unlikely(timer_tsc >= timer_period))
        //         {
        //             /* reset the timer */
        //             timer_tsc = 0;
        //             RecordBench(cur_tsc);
        //             // ssPrintBench();
        //             // upPrintBench();
        //             // hsPrintBench();
        //         }
        //     }
        //     prev_tsc = cur_tsc;
        // }

        /*
		 * Read packet from RX queues
		 */
        portid = srcPort;
        nb_rx = rte_eth_rx_burst(
            portid, workerDataList[workerId].rxQueue, workerDataList[workerId].packetBurst, MAX_PKT_BURST);
        // workerDataList[workerId].stat.rx += nb_rx;
        workerDataList[workerId].burstSize = nb_rx;

        // prevent unnecessary pkey overhead
        // IMPORTANT: careful when change code below this
        if (nb_rx == 0)
        {
            continue;
        }

        // postRx = rte_rdtsc();

        // switch into the first MOON in the chain
        // that MOON will switch into the next one instead of return here
        workerDataList[workerId].current = 0;
        MoonSwitch(workerId);
        // up_clk0 = rte_rdtsc();
        DisablePkey(0);
        // up_clk1 = rte_rdtsc();
        // up_idx = 0;
        // up_array[up_idx++] += up_clk1 - up_clk0;
        // up_sum += up_clk1 - up_clk0;
        // upCounter++;
        // now we are back from the last MOON, the packet burst is done!

        // preTx = rte_rdtsc();
        // workerDataList[workerId].stat.latency += (double)(preTx - postRx) * 1000 * 1000 / rte_get_tsc_hz();
        // workerDataList[workerId].stat.batch++;


        sent = rte_eth_tx_burst(
            // dstPort, workerDataList[workerId].txQueue,
            srcPort, workerDataList[workerId].txQueue,
            workerDataList[workerId].packetBurst, workerDataList[workerId].burstSize);
        if (sent)
            workerDataList[workerId].stat.tx += sent;
        for (int i = 0;i < sent; i++)
            workerDataList[workerId].stat.bytes += rte_pktmbuf_pkt_len(workerDataList[workerId].packetBurst[i]);
        // if (unlikely(sent < nb_rx))
        // {
        //     rte_pktmbuf_free_bulk(workerDataList[workerId].packetBurst + sent, nb_rx-sent);
        // }
    }
    printf("[RunMoon] worker on lcore$%d exit\n", lcore_id);
    return 0;
}

// the following functions are executed in MOON env
// i.e. on private stack with private heap
void InitMoon()
{
    // in multi-core settings, we cannot assume argv remain unchanged given 
    // it's not 'const'. This might cause minor memory leak, though
    int argc = CONFIG[loading.configIndex].argc;
    char **argv = malloc((argc+1) * sizeof(char *));
    memcpy(argv, CONFIG[loading.configIndex].argv, argc * sizeof(char *));
    argv[argc] = NULL;
    optind = 1;
    // char *argv[0];
    printf("[InitMoon] calling moonStart\n");
    loading.moonStart(argc, argv);
    // loading.moonStart(0, argv);
    printf("[InitMoon] nf exited before blocking on packets, intended?\n");
    exit(0);
}

struct ThreadClosure
{
    void *(*start)(void *);
    void *restrict arg;
};

static void *ThreadMain(void *data)
{
    per_lcore__lcore_id = 0;
    SetHeap(loading.heapStart, loading.heapSize, loading.instanceId);
    CrossThreadRestoreStack();
    struct ThreadClosure *closure = data;
    printf("finished recover TLS for pthread of function %p\n", closure->start);
    return closure->start(closure->arg);
}

int PthreadCreate(
    pthread_t *restrict thread,
    const pthread_attr_t *restrict _attr,
    void *(*start_routine)(void *),
    void *restrict arg)
{
    if (!loading.inProgress)
    {
        fprintf(stderr, "[PthreadCreate] not allowed after loading\n");
        abort();
    }
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    loading.threadStackIndex += 1;
    if (loading.threadStackIndex == NUMBER_STACK)
    {
        fprintf(stderr, "too many thread in one MOON\n");
        abort();
    }
    void *threadStackStart = loading.stackStart + STACK_SIZE * loading.threadStackIndex;
    printf("set thread stack to %p size %#lx\n", threadStackStart, STACK_SIZE);
    pthread_attr_setstack(&attr, threadStackStart, STACK_SIZE);
    struct ThreadClosure *closure = malloc(sizeof(struct ThreadClosure));
    closure->start = start_routine;
    closure->arg = arg;
    CrossThreadSaveStack();
    return pthread_create(thread, &attr, ThreadMain, closure);
}

// core part of packet IO
int PcapLoop(pcap_t *p, int cnt, pcap_handler callback, u_char *user)
{
    if (cnt > 0)
    {
        fprintf(stderr, "PcapLoop not support cnt > 0\n");
        abort();
    }
    unsigned int workerId;
    for (;;)
    {
        if (loading.inProgress)
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
            // *moonDataList[workerDataList[workerId].current].workers[workerId].extraLowPtr = (uintptr_t)packet;
            // *moonDataList[workerDataList[workerId].current].workers[workerId].extraHighPtr = (uintptr_t)packet + header.caplen;
            callback(user, &header, (u_char *)packet);
        }
    }
}

int PcapDispatch(pcap_t *p, int cnt, pcap_handler callback, u_char *user)
{
    if (cnt != 1)
    {
        fprintf(stderr, "PcapDispatch not cnt != 1\n");
        abort();
    }
    unsigned int workerId;
    if (loading.inProgress)
    {
        StackSwitch(-1);
        workerId = rte_lcore_id();
        workerDataList[workerId].pcapNextIndex = 0;
    }
    workerId = rte_lcore_id();
    if (workerDataList[workerId].pcapNextIndex >= workerDataList[workerId].burstSize)
    {
        workerDataList[workerId].pcapNextIndex = 0;
        workerDataList[workerId].current = moonDataList[workerDataList[workerId].current].switchTo;
        MoonSwitch(workerId);
    }
    int i = workerDataList[workerId].pcapNextIndex;
    workerDataList[workerId].pcapNextIndex += 1;
    rte_prefetch0(rte_pktmbuf_mtod(workerDataList[workerId].packetBurst[i], void *));
    struct pcap_pkthdr header;
    header.len = rte_pktmbuf_pkt_len(workerDataList[workerId].packetBurst[i]);
    header.caplen = rte_pktmbuf_data_len(workerDataList[workerId].packetBurst[i]);
    memset(&header.ts, 0x0, sizeof(header.ts)); // todo
    uintptr_t *packet = rte_pktmbuf_mtod(workerDataList[workerId].packetBurst[i], uintptr_t *);
    callback(user, &header, (u_char *)packet);
    return 1;
}

const u_char *PcapNext(pcap_t *p, struct pcap_pkthdr *h)
{
    unsigned int workerId;
    if (loading.inProgress)
    {
        printf("[PcapNext] initialization finish\n");
        StackSwitch(-1);
        printf("[PcapNext] start first burst\n");
        workerId = rte_lcore_id();
        workerDataList[workerId].pcapNextIndex = 0;
    }
    workerId = rte_lcore_id();
    if (workerDataList[workerId].pcapNextIndex >= workerDataList[workerId].burstSize)
    {
        workerDataList[workerId].pcapNextIndex = 0;
        workerDataList[workerId].current = moonDataList[workerDataList[workerId].current].switchTo;
        MoonSwitch(workerId);
    }
    int i = workerDataList[workerId].pcapNextIndex;
    workerDataList[workerId].pcapNextIndex += 1;
    rte_prefetch0(rte_pktmbuf_mtod(workerDataList[workerId].packetBurst[i], void *));
    struct pcap_pkthdr header;
    header.len = rte_pktmbuf_pkt_len(workerDataList[workerId].packetBurst[i]);
    header.caplen = rte_pktmbuf_data_len(workerDataList[workerId].packetBurst[i]);
    memset(&header.ts, 0x0, sizeof(header.ts)); // todo
    uintptr_t *packet = rte_pktmbuf_mtod(workerDataList[workerId].packetBurst[i], uintptr_t *);
    // *moonDataList[workerDataList[workerId].current].workers[workerId].extraLowPtr = (uintptr_t)packet;
    // *moonDataList[workerDataList[workerId].current].workers[workerId].extraHighPtr = (uintptr_t)packet + header.caplen;
    *h = header;
    return (u_char *)packet;
}

uint16_t RxBurst(void *rxq, struct rte_mbuf **rx_pkts, uint16_t nb_pkts)
{
    unsigned int workerId;
    if (loading.inProgress)
    {
        loading.isDpdkMoon = 1;
        StackSwitch(-1);
        workerId = rte_lcore_id();
    }
    else
    {
        workerId = rte_lcore_id();
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

int PthreadCondTimedwait(
    pthread_cond_t *restrict cond,
    pthread_mutex_t *restrict mutex,
    const struct timespec *restrict abstime)
{
    if (rte_lcore_id() != 0)
    {
        fprintf(stderr, "not allowed to wait on worker thread\n");
        abort();
    }
    return pthread_cond_timedwait(cond, mutex, abstime);
}

int PthreadCondWait(
    pthread_cond_t *restrict cond,
    pthread_mutex_t *restrict mutex)
{
    if (rte_lcore_id() != 0)
    {
        fprintf(stderr, "not allowed to wait on worker thread\n");
        abort();
    }
    return pthread_cond_wait(cond, mutex);
}