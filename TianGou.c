#include "TianGou.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <rte_mempool.h>


Interface interface;

// pcap


// dpdk

// two devices for src/dst ports
// wrapper for rx/tx burst directly goes into runtime and reigster on this
// array, because the functions are inlined in DPDK
// this cannot go into interface, because I cannot "sync" it when assign to
// fields in interface
