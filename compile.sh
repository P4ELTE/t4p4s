#!/bin/bash

if [ "$#" -lt 1 ]; then
    (>&2 echo "Usage: $0 <P4 file>")
    exit 1
fi

P4_SOURCE=$1

if [ ! -f "$P4_SOURCE" ]; then
    (>&2 echo "Error: file not found: ${P4_SOURCE}")
    (>&2 echo "Usage: $0 <P4 file>")
    exit 1
fi


P4_BASENAME=$(basename ${1%.*})
P4DPDK_TARGET_DIR=${P4DPDK_TARGET_DIR-"build/$P4_BASENAME"}


python src/compiler.py "${P4_SOURCE}" "${P4DPDK_TARGET_DIR}/src_hardware_indep"


cp -n makefiles/*.mk "${P4DPDK_TARGET_DIR}/"
if [ ! -f "${P4DPDK_TARGET_DIR}/Makefile" ]; then
    cat makefiles/Makefile | sed -e "s/example_dpdk1/${P4_BASENAME}/g" > "${P4DPDK_TARGET_DIR}/Makefile"
else
    echo Makefile already exists, skipping
fi
