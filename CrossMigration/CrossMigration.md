# Inter-process migration branch summary

This branch implements a toy version of inter-process live migration scheme based on iterative copying,
mainly to prove that the ability to control address space enable us with the *ability* to perform fast migration across machines.

## Method

With ASLR disabled, two processes with identical settings will run as exactly the same memory layouts.
Yet currently due to the inmature custom loader, we still resort to `dlopen` for loading MOONs.
Therefore, we are forced to boost the source and destiniation process with *exactly the same argument.*

In this experiment, I setup the same chain (L2Fwd -> Rubik) over two processes,
and before they run, adjust the topo of dest chain to currently ignore Rubik.
In this way we create a setting that resembles simple Rubik instance running,
and migrating it from the source process to the destination one.

## Commands

To run the source process:

```
sudo ./RunMoon/go.sh -m 2 -m 7 -c 0x6 -v 2
```

and the destination:

```
./RunMoon/go.sh -m 2 -m 7 -c 0x18 -v 3 -n
```

First, we are running L2Fwd and Rubik, so the moon index is obvious.

The core mask here is a little tricky:
If I generate packets from the another to current server,
from `htop` I observe that the core index ranging from 1 to 48, 
and all the **even** cores have a 100% utilization.
This is because the incoming packets constantly interrupt the CPUs.
Because the NIC is installed on socket 1, it will force all threads on this socket to handle interrupts.
Thus, ideally speaking, if we want optimal results, we should use even cores as workers,
and the mask will look like `0b11` or `0x3`.

`-v` specifies VF index. In this experiment, we use VF to setup 'auto forwarding':
we hard code the MAC of downstream VF into upstream neptune, such that 
VF2 will send packets to VF3 p2p.

`-n` specifies 'needs mapping', an indicator of destination process.
In `RunMoon/Main.c` I hard code a lot based on this flag.

## Results

Remember to check whether Release is on in CMake.

If using even cores as workers, before migration we will get ~13G, but will stably drops to ~5.5G after migration.
As a comparison, if we use odd worker cores, before migration we will get ~12G (ranging from 11.5 to 12.2),
and get a ~2-2.5% increase after migration.

Under Rubik, the dirty pages usually converge at ~530, that is to say, when dirty page number reaches this limit,
it will increase slowly, e.g. one in a second.

## Iterative strategy

### When to start a new iteration

The basis of iterative migration is to periodically retrieve the write permission granting to previous 'dirty pages',
and find an ideal timing to copy the pages such that the down time can be reduced to lowest.

In my implementation, the dirty page are stored in an fixed size array (current at 1024).
There are three pointers (lb, mb, ub) pointing to three locations:
start of pages that permission is granted,
end of pages that permission is granted, and
the current head of dirty pages.

The first pointer is written by main core and the other two is written by the worker.
When the worker decides to grant permission (current the policy is `ub - mb > 100`),
it first check whether the main core finishes it job:
if `lb < mb`, the main is still busy dumping the pages from previous iteration.,
or if the max iteration time is met,
or if the main core has decided that the number of dirty pages converge.
Otherwise, the worker retrieves permission from `[mb, ub]`,
and track new dirty pages from `ub+1`.

### When to decide blocking source process

Previously, I felt that the optimal timing can be calculated solely from the source process,
minimizing the pages to dump.
However, the real condition is that dumping is considered finished long before pre-loading finish.
In this way the source process would wait (and block) a long time until the destination process is finished.

To this end, I introduce another shared memory object called `reversedFd`, whose purpose is to notify 
the source process about the finish of destinatio pre-load.
The source process would not try to block the worker until this flag is received.
Ideally, pre-load can happen for several rounds, hopefully the source can find a satisfactory value.
Currently, when source converges, it will halt pre-copy stage,
and wait for the destiniation to complete.
Then the source will start to block the worker and proceed to block copy stage. 