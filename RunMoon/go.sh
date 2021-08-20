#!/bin/bash

set -e

allMoonDirs=(
    "depreated"
    "depreated"
    "/libs/L2Fwd"
    "/libs/fastclick"
    "/libs/Libnids"
    "/libs/ndpi"
    "/libs/NetBricks"
    "/libs/rubik"
)

absoluteDir="/home/hypermoon/neptune-yh"
moonList=()

while [[ "$#" -gt 0 ]]; do
    case $1 in
        -m|--moon) moonList+=("$2"); shift; shift;;
        -c|--core) numCore="$2"; shift; shift;;
        -p|--pku) enablePku="--pku"; shift;;
        *) echo "bad argument $1"; exit 1;;
    esac
done

# echo "c = $((numCore))"
coreMask=$(( (1<<(numCore+1))-1 ))
coreMask=$( printf "%x" $coreMask )
# echo "core mask=$coreMask"

libraryPath="LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:"
moonConfig=""

for i in "${!moonList[@]}"; do
    currDir=${allMoonDirs[${moonList[$i]}]}
    # echo "moonList[$i] = ${moonList[$i]}, currDir = $currDir"
    if [ -d "$absoluteDir$currDir" ]
    then
        moonConfig="$moonConfig ${moonList[$i]}"
        libraryPath="$libraryPath$absoluteDir$currDir:"
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
cmd="$perm $libraryPath $progName -c 0x$coreMask -- $tiangouPath $enablePku $moonConfig"
echo $cmd
eval $cmd