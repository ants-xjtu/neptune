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

// we are doing a chain, so no static moon
// const int MOON_ID = 0;
static const char *DONE_STRING = "\xe2\x86\x91 done";
#define RTE_TEST_RX_DESC_DEFAULT 1024
#define RTE_TEST_TX_DESC_DEFAULT 1024
static uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
static uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;

int (*moonStart)(int argc, char *argv[]);
void initMoon();
volatile bool force_quit;
void l2fwd_main_loop(void);
void LoadMoon(char *, int);
int MoonNum;       //the total number of moons, temporary use
void *nf_state[2]; //the state information of the list of moon, temporary use

void signal_handler(int signum);
void check_all_ports_link_status(uint32_t port_mask);

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

void SetupDpdk();

int PcapLoop(pcap_t *p, int cnt, pcap_handler callback, u_char *user);

#define MAX_PKT_BURST 32
struct rte_mbuf *packetBurst[MAX_PKT_BURST];
unsigned int burstSize;

#endif