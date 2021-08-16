#include "Common.h"

struct BenchRecord
{
    double avgPps;
    double avgMbps;
    double avgCopyCycles;
    double avgCompareCycles;
    uint64_t tsc;
};
struct BenchRecord workerRecordList[MAX_WORKER_ID];

void RecordBench(uint64_t currentTsc)
{
    unsigned int workerId = rte_lcore_id();
    struct l2fwd_port_statistics *stat = &workerDataList[workerId].stat;
    double pps = (double)stat->tx / numberTimerSecond / 1000;
    double bps = (double)stat->bytes / numberTimerSecond / 1000 / 1000 * 8;
    double copyCycle = (double)stat->cpCycle / stat->rx;
    // double cmpCycle = (double)stat->cmpCycle / stat->rx;
    stat->tx = 0;
    stat->bytes = 0;
    /* add to evaluate copy overhead */
    stat->rx = 0;
    stat->cpCycle = 0;
    // stat->cmpCycle = 0;
    // less equal to include slow start (pps == 0, prevAvg == 0) case
    if (pps <= 0.8 * stat->prevAvg)
    {
        printf("[worker$%02u] ignore bad record\n", workerId);
        return;
    }
    stat->cycle[stat->cycleIndex % CYCLE_SIZE] = pps;
    stat->Mcycle[stat->cycleIndex % CYCLE_SIZE] = bps;
    stat->cycleIndex += 1;

    double sum = 0.0;
    double Msum = 0.0;
    int count = stat->cycleIndex >= CYCLE_SIZE ? CYCLE_SIZE : stat->cycleIndex;
    for (int i = 0; i < count; i += 1)
    {
        sum += stat->cycle[i];
        Msum += stat->Mcycle[i];
    }
    workerRecordList[workerId].avgPps = sum / count;
    workerRecordList[workerId].avgMbps = Msum / count;
    workerRecordList[workerId].avgCopyCycles = copyCycle;
    // workerRecordList[workerId].avgCompareCycles = cmpCycle;
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
    double bps = 0.0;
    double cycle = 0.0;
    // double cmpcycle = 0.0;
    RTE_LCORE_FOREACH_WORKER(workerId)
    {
        if (workerRecordList[workerId].tsc + timer_period < currentTsc)
        {
            return;
        }
        pps += workerRecordList[workerId].avgPps;
        bps += workerRecordList[workerId].avgMbps;
        cycle += workerRecordList[workerId].avgCopyCycles;
        // cmpcycle += workerRecordList[workerId].avgCompareCycles;
    }
    // printf("pps: %fK\tbps:%fM\tavgCopyCycle:%f\tavgCmpCycle:%f\n", pps, bps, cycle, cmpcycle);
    // printf("pps: %fK\tbps:%fM\tavgCopyCycle:%f\n", pps, bps, cycle);
    printf("pps: %fK\tbps:%fM\n", pps, bps);
    prevPrintTsc = currentTsc;
}