## intro

This file will guide you through the isolation-recovery experiment.

## Preparation: a faulty NF

I choose libnids and force it to access an address outside it own heap (MOON),
such that the illegal access will be catched by the PKU.

The binary and source for this NF can be found under `libs/Libnids-faulty` and .

The basic notion is that this NF triggers a sigfault every few packets.
The reason why I choose packet count instead of a fixed time interval is that 
this might be more stable: the NF won't starts counting again if the signal takes a long time to deal with.