#!/bin/bash

print_usage_and_exit() {
    (>&2 echo "Usage: $0 <P4 file> [controller name] [controller params file] -- <options for compiled switch>")
    exit 1
}

if [ $# -lt 1 ]; then
    print_usage_and_exit
fi

if [ -z ${P4_TARGET+x} ]; then
    P4_TARGET=dpdk
fi

P4_SOURCE=$1
shift

if [ ! -f "$P4_SOURCE" ]; then
    (>&2 echo "Error: file not found: ${P4_SOURCE}")
    print_usage_and_exit
fi

echo "-------------------- Compilation P4 -> C"

./compile.sh $P4_SOURCE
if [ "$?" -ne 0 ]; then
    exit 1
fi

echo "-------------------- Compilation C -> executable"

if [ ! -z ${P4_DPDK_VARIANT+x} ]; then
    echo "Note: using variant ${P4_DPDK_VARIANT}"

    # If a no-NIC variant is used, the code has to be changed a bit, too.
    if [[ ${P4_DPDK_VARIANT} == *"no_nic"* ]]
    then
        export P4_GCC_OPTS="${P4_GCC_OPTS} -DFAKEDPDK"
    fi
fi

if [ ! -z ${P4_GCC_OPTS+x} ]; then
    echo "Note: extra C options: ${P4_GCC_OPTS}"
fi

P4_BASENAME=$(basename ${P4_SOURCE%.*})
cd ./build/$P4_BASENAME
make -j
ERROR_CODE=$?
if [ "$ERROR_CODE" -ne 0 ]; then
    echo C compilation failed with error code $ERROR_CODE
    exit 1
fi

cd - >/dev/null

echo "-------------------- Starting execution"

./run.sh ./build/$P4_BASENAME/$P4_TARGET/$P4_BASENAME $*
