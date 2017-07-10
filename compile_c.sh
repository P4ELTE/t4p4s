#!/bin/bash

if [ "$#" -lt 1 ]; then
    (>&2 echo "Usage: $0 <P4 file>")
    exit 1
fi

P4_SOURCE=$1
shift

if [ ! -z ${P4DPDK_VARIANT+x} ]; then
    if [[ "${@#--verbose}" != "$@" ]]; then
        echo "Note: using variant ${P4DPDK_VARIANT}"
    fi

    # If a no-NIC variant is used, the code has to be changed a bit, too.
    if [[ ${P4DPDK_VARIANT} == *"no_nic"* ]]
    then
        export P4_GCC_OPTS="${P4_GCC_OPTS} -DFAKEDPDK"
    fi
fi

if [ ! -z ${P4_GCC_OPTS+x} ]; then
    if [[ "${@#--verbose}" != "$@" ]]; then
        echo "Note: extra C options: ${P4_GCC_OPTS}"
    fi
fi


P4_BASENAME=$(basename ${P4_SOURCE%.*})
cd ./build/$P4_BASENAME
make -j
ERROR_CODE=$?
if [ "$ERROR_CODE" -ne 0 ]; then
    exit "$ERROR_CODE"
fi

cd - >/dev/null
