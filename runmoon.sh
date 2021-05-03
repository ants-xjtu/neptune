#!/bin/bash

PKU_FLAG=""
PKU_FLAG=" --pku"

MOON_NAME="Libnids"
# MOON_NAME="Prads"

SFI_FLAG=""
SFI_FLAG="_NoSFI"

COUNT=10  # how many MOONs to be chained
CORE_MASK=0x1f  # worker count = <number of f> * 4

MOON_SO="./build/libMoon_${MOON_NAME}${SFI_FLAG}.so"
SUBCOMMAND="printf '${MOON_SO} %.0s' {1..${COUNT}}"
MOON_ARGS=`eval ${SUBCOMMAND}`
COMMAND="sudo ./build/RunMoon -c ${CORE_MASK} -- ./build/libTianGou.so${PKU_FLAG} ${MOON_ARGS}"
echo $COMMAND
eval $COMMAND