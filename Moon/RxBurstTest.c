#include <rte_ethdev.h>

#define MAX_PKT_BURST 32

static const uint16_t portId = 0, queueId = 0;

int main()
{
    struct rte_mbuf *rx_pkts[MAX_PKT_BURST];
    printf("[RxBurstTest] call rte_eth_rx_burst now\n");
    rte_eth_rx_burst(portId, queueId, rx_pkts, MAX_PKT_BURST);
    return 0;
}