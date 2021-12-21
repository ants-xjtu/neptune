#!/bin/bash

set -e

allMoonDirs=(
    "depreated"
    "depreated"
    "/libs/L2Fwd"
    "/libs/fastclick"
    "/libs/Libnids"
    "/libs/ndpi-new"
    "/libs/NetBricks"
    "/libs/rubik-new"
    "/nfd/five"
    "/nfd/napt"
    "/nfd/hhd"
    "/nfd/firewall"
    "/nfd/ssd"
    "/nfd/udpfm"
    "/onvm/scan"
    "/onvm/firewall"
    "/onvm/encrypt"
    "/onvm/decrypt"
    "/libs/acl-fw"
)

absoluteDir="/home/hypermoon/neptune-yh"
moonList=()

while [[ "$#" -gt 0 ]]; do
    case $1 in
        -m|--moon) moonList+=("$2"); shift; shift;;
        -c|--core) coreMask="$2"; shift; shift;;
        -p|--pku) enablePku="--pku"; shift;;
        -g|--debug) debugFlag="gdb --args"; shift;;
        *) echo "bad argument $1"; exit 1;;
    esac
done

# echo "c = $((numCore))"
if [ ${coreMask:0:2} != "0x" ]
then
    echo "please input a core mask"
    exit 1;
fi

detectCore=$coreMask
numCore=0

# ignore main core which is always on lcore 0
detectCore=$((detectCore>>1))
while [ ! "$detectCore" -eq "0" ]
do
    # echo $detectCore
    if (( $detectCore&1 != 0 ))
    then
        numCore=$((numCore+1))
    fi
    detectCore=$((detectCore>>1))
done
# coreMask=$(( (1<<(numCore+1))-1 ))
# coreMask=$( printf "%x" $coreMask )
# echo "core mask=$coreMask"

libraryPath="LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:"
moonConfig=""

# TODO: clear this message
# WARNING: if you want to duplicate MOON on a single SFC, then do not use
#   multi-core and vice versa. These issue will disappear once we switch
#   to dlmopen neatly.

for i in "${!moonList[@]}"; do
    currDir=${allMoonDirs[${moonList[$i]}]}
    # echo "moonList[$i] = ${moonList[$i]}, currDir = $currDir"
    ctr=0
    for (( j=0; j<$i; j++ )); do
        if [ ${moonList[$i]} == ${moonList[$j]} ]
        then
            ctr=$(( $ctr+1 ))
        fi
    done
    if [ -d "$absoluteDir$currDir" ]
    then
        moonConfig="$moonConfig ${moonList[$i]}"
        if [ $ctr == 0 ]
        then
            libraryPath="$libraryPath$absoluteDir$currDir:"
        else
            libraryPath="$libraryPath$absoluteDir$currDir$(( ctr+1 )):"
        fi
    else
        echo "$currDir: dir not exist";
        exit 1;
    fi
done

for (( j=1; j<$numCore; j++ )); do
    for i in "${!moonList[@]}"; do
        currDir=${allMoonDirs[${moonList[$i]}]}
        # echo "moonList[$i] = ${moonList[$i]}, currDir = $currDir"
        if [ -d "$absoluteDir$currDir$(( j+1 ))" ]
        then
            # moonConfig="$moonConfig ${moonList[$i]}"
            libraryPath="$libraryPath$absoluteDir$currDir$(( j+1 )):"
        else
            echo "$currDir$(( j+1 )): dir not exist";
            exit 1;
        fi
    done
done

buildPath="/home/hypermoon/neptune-yh/build"
libraryPath="$libraryPath$buildPath"
# echo "moonConfig = $moonConfig"
# echo "library path = $libraryPath"

perm="sudo"
progName="./build/RunMoon"
tiangouPath="./build/libTianGou.so"
# cmd="$perm $libraryPath $debugFlag $progName -c 0x$coreMask -- $tiangouPath $enablePku $moonConfig"
cmd="$perm $libraryPath $debugFlag $progName -c $coreMask -- $tiangouPath $enablePku $moonConfig"
echo $cmd
eval $cmd