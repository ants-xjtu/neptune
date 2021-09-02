#ifndef NEPTUNE_RUNMOON_H
#define NEPTUNE_RUNMOON_H

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
#include <sys/mman.h>

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

static const size_t STACK_SIZE = 32ul << 20; // 32MB
static const int NUMBER_STACK = 4;
static const size_t MOON_SIZE = 1ul << 30; // 32GB
// Heap size = MOON_SIZE - NUMBER_STACK * STACK_SIZE - library.length

// static const char *DONE_STRING = "\xe2\x86\x91 done";
#define RTE_TEST_RX_DESC_DEFAULT 1024
#define RTE_TEST_TX_DESC_DEFAULT 1024
static uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
static uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;
#define MAX_PKT_BURST 32
#define MAX_WORKER_ID 96
#define CYCLE_SIZE 40

// global variables of runtime supervisor & workers
// constant that will only be set once during runtime init
int runtimePkey;
int enablePku;
// loading time variable
// when these variables are set/accessed, only one MOON is initializing
// on runtime supervisor's thread, so do not need to think about thread
// safety
struct Loading
{
    int inProgress;
    int instanceId;
    int (*moonStart)(int argc, char *argv[]);
    int configIndex;
    int isDpdkMoon;
    void *heapStart;
    size_t heapSize;
    void *stackStart;
    int threadStackIndex;
    // how many core has finished processing for this moon
    int workerCount;
};
struct Loading loading;
// moonDataList[] is only set during loading/reconfiguration(WIP), and
// workers will only read them
// instance data inside moonDataList[].workers[] is allocated per MOON per worker
// so each worker only access its own index
struct MoonData
{
    int id; // -1 for end of MOONs
    int config;
    int pkey;
    int switchTo;
    struct
    {
        int instanceId; // for stack/heap registration
        uintptr_t *extraLowPtr, *extraHighPtr;
    } workers[MAX_WORKER_ID];
};
struct MoonData moonDataList[16];
// worker data is accessed by each worker on its own index
struct l2fwd_port_statistics
{
    uint64_t tx;
    uint64_t rx;
    uint64_t dropped;
    uint64_t bytes;
    double cycle[CYCLE_SIZE];
    double Mcycle[CYCLE_SIZE];
    int cycleIndex;
    double prevAvg;
    uint64_t cpCycle;
    uint64_t cmpCycle;
} __rte_cache_aligned;
// struct l2fwd_port_statistics port_statistics;
struct WorkerData
{
    int current; // MOON id
    int rxQueue, txQueue;
    struct rte_mbuf *packetBurst[MAX_PKT_BURST];
    unsigned int burstSize;
    int pcapNextIndex;

    struct l2fwd_port_statistics stat;
};
struct WorkerData workerDataList[MAX_WORKER_ID];

// forward decalrations for runtime main
void InitMoon();
int MainLoop(void *);
void LoadMoon(char *, int, int);
int PcapLoop(pcap_t *p, int cnt, pcap_handler callback, u_char *user);
const u_char *PcapNext(pcap_t *p, struct pcap_pkthdr *h);
int PcapDispatch(pcap_t *p, int cnt, pcap_handler callback, u_char *user);
uint16_t RxBurst(void *rxq, struct rte_mbuf **rx_pkts, uint16_t nb_pkts);
uint16_t TxBurst(void *txq, struct rte_mbuf **tx_pkts, uint16_t nb_pkts);
int PthreadCreate(
    pthread_t *restrict thread,
    const pthread_attr_t *restrict attr,
    void *(*start_routine)(void *),
    void *restrict arg);
int PthreadCondTimedwait(
    pthread_cond_t *restrict cond,
    pthread_mutex_t *restrict mutex,
    const struct timespec *restrict abstime);
int PthreadCondWait(
    pthread_cond_t *restrict cond,
    pthread_mutex_t *restrict mutex);

// support library APIs and global shared with main
volatile bool force_quit;
uint16_t srcPort, dstPort;
uintptr_t mbufLow, mbufHigh;
void SetupDpdk();
void RedirectEthDevices(struct rte_eth_dev *devices);

// bench
uint64_t numberTimerSecond;
extern uint64_t timer_period;
void RecordBench(uint64_t);
void PrintBench();

// pkey
void UpdatePkey(unsigned int workerId);
void DisablePkey(int force);

#endif