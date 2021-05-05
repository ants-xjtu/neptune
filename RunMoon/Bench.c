#include "Common.h"

struct BenchRecord
{
    double avgPps;
    uint64_t tsc;
};
struct BenchRecord workerRecordList[MAX_WORKER_ID];

void RecordBench(uint64_t currentTsc)
{
    unsigned int workerId = rte_lcore_id();
    struct l2fwd_port_statistics *stat = &workerDataList[workerId].stat;
    double pps = (double)stat->tx / numberTimerSecond / 1000;
    stat->tx = 0;
    // less equal to include slow start (pps == 0, prevAvg == 0) case
    if (pps <= 0.8 * stat->prevAvg)
    {
        printf("[worker$%02u] ignore bad record\n", workerId);
        return;
    }
    stat->cycle[stat->cycleIndex % CYCLE_SIZE] = pps;
    stat->cycleIndex += 1;

    double sum = 0.0;
    int count = stat->cycleIndex >= CYCLE_SIZE ? CYCLE_SIZE : stat->cycleIndex;
    for (int i = 0; i < count; i += 1)
    {
        sum += stat->cycle[i];
    }
    workerRecordList[workerId].avgPps = sum / count;
    workerRecordList[workerId].tsc = currentTsc;
    // printf("[worker$%02d] pps: %.3fK\n", workerId, sum / count);
}

static uint64_t prevPrintTsc;

void PrintBench()
{
    uint64_t currentTsc = rte_rdtsc();
    if (prevPrintTsc + timer_period > currentTsc)
    {
        return;
    }
    unsigned int workerId;
    double pps = 0.0;
    RTE_LCORE_FOREACH_WORKER(workerId)
    {
        if (workerRecordList[workerId].tsc + timer_period < currentTsc)
        {
            return;
        }
        pps += workerRecordList[workerId].avgPps;
    }
    printf("pps: %fK\n", pps);
    prevPrintTsc = currentTsc;
}