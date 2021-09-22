#include "Common.h"

struct BenchRecord
{
    double avgPps;
    double avgMbps;
    double avgLatency;
    uint64_t tsc;
};
struct BenchRecord workerRecordList[MAX_WORKER_ID];

void RecordBench(uint64_t currentTsc)
{
    unsigned int workerId = rte_lcore_id();
    struct l2fwd_port_statistics *stat = &workerDataList[workerId].stat;
    double pps = (double)stat->tx / numberTimerSecond / 1000;
    double bps = (double)stat->bytes / numberTimerSecond / 1000 / 1000 * 8;
    double latency = stat->latency / numberTimerSecond / stat->batch;
    stat->tx = 0;
    stat->bytes = 0;
    stat->latency = 0;
    stat->batch = 0;
    // less equal to include slow start (pps == 0, prevAvg == 0) case
    if (pps <= 0.8 * stat->prevAvg)
    {
        printf("[worker$%02u] ignore bad record\n", workerId);
        return;
    }
    stat->cycle[stat->cycleIndex % CYCLE_SIZE] = pps;
    stat->Mcycle[stat->cycleIndex % CYCLE_SIZE] = bps;
    stat->Lcycle[stat->cycleIndex % CYCLE_SIZE] = latency;
    stat->cycleIndex += 1;

    double sum = 0.0;
    double Msum = 0.0;
    double Lsum = 0.0;
    int count = stat->cycleIndex >= CYCLE_SIZE ? CYCLE_SIZE : stat->cycleIndex;
    for (int i = 0; i < count; i += 1)
    {
        sum += stat->cycle[i];
        Msum += stat->Mcycle[i];
        Lsum += stat->Lcycle[i];
    }
    workerRecordList[workerId].avgPps = sum / count;
    workerRecordList[workerId].avgMbps = Msum / count;
    workerRecordList[workerId].avgLatency = Lsum / count / (rte_lcore_count() - 1);
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
    double latency = 0.0;
    // double cycle = 0.0;
    // double cmpcycle = 0.0;
    RTE_LCORE_FOREACH_WORKER(workerId)
    {
        if (workerRecordList[workerId].tsc + timer_period < currentTsc)
        {
            return;
        }
        pps += workerRecordList[workerId].avgPps;
        bps += workerRecordList[workerId].avgMbps;
        latency += workerRecordList[workerId].avgLatency;
        // helper to see the load on each core
        // printf("[worker$%02d] pps: %fK\tbps:%fM\n", workerId,
        //      workerRecordList[workerId].avgPps, workerRecordList[workerId].avgMbps);
    }
    printf("pps: %fK\tbps:%fM\tlatency:%fus\n", pps, bps, latency);
    prevPrintTsc = currentTsc;
}