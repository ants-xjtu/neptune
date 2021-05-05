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
static const size_t MOON_SIZE = 32ul << 30;  // 32GB
// Heap size = MOON_SIZE - STACK_SIZE - library.length

static const char *DONE_STRING = "\xe2\x86\x91 done";
#define RTE_TEST_RX_DESC_DEFAULT 1024
#define RTE_TEST_TX_DESC_DEFAULT 1024
static uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
static uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;

// global variables of runtime main
// most of them are passing information between runtime/parivate stacks
int (*moonStart)(int argc, char *argv[]);
// struct rte_eth_dev_tx_buffer *txBuffer;
Interface *interface;
#define MAX_PKT_BURST 32
int runtimePkey;
#define MAX_WORKER_ID 96
struct MoonData
{
    int id; // -1 for end of MOONs
    int pkey;
    int switchTo;
    struct
    {
        int instanceId; // for stack/heap registration
        uintptr_t *extraLowPtr, *extraHighPtr;
    } workers[MAX_WORKER_ID];
};
struct MoonData moonDataList[16];
int enablePku;
int loading;
int isDpdkMoon;
#define CYCLE_SIZE 40
struct l2fwd_port_statistics
{
    uint64_t tx;
    uint64_t rx;
    uint64_t dropped;
    double cycle[CYCLE_SIZE];
    int cycleIndex;
    double prevAvg;
} __rte_cache_aligned;
// struct l2fwd_port_statistics port_statistics;
struct WorkerData
{
    int current; // MOON id
    int rxQueue, txQueue;
    struct rte_mbuf *packetBurst[MAX_PKT_BURST];
    unsigned int burstSize;

    struct l2fwd_port_statistics stat;
};
struct WorkerData workerDataList[MAX_WORKER_ID];

// forward decalrations for runtime main
void InitMoon();
int MainLoop(void *);
void LoadMoon(char *, int);
int PcapLoop(pcap_t *p, int cnt, pcap_handler callback, u_char *user);
uint16_t RxBurst(void *rxq, struct rte_mbuf **rx_pkts, uint16_t nb_pkts);
uint16_t TxBurst(void *txq, struct rte_mbuf **tx_pkts, uint16_t nb_pkts);

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