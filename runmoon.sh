#!/bin/bash

PKU_FLAG=" --pku"
# PKU_FLAG=""
MOON_SO="./build/libMoon_Libnids_NoSFI.so"
COUNT=5

SUBCOMMAND="printf '${MOON_SO} %.0s' {1..${COUNT}}"
MOON_ARGS=`eval ${SUBCOMMAND}`
COMMAND="sudo ./build/RunMoon -- ./build/libTianGou.so${PKU_FLAG} ${MOON_ARGS}"
echo $COMMAND
eval $COMMAND