#include "Common.h"

#define CYCLE_SIZE 40
double cycle[CYCLE_SIZE], prevAvg = -1;
int cycleIndex = 0;
#define AVG_DELTA_CYCLE_SIZE 24
int avgDeltaCycle[AVG_DELTA_CYCLE_SIZE];
int avgDeltaCycleIndex = 0;

void PrintBench()
{
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