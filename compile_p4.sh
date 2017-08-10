#!/bin/bash

if [ "$#" -lt 1 ]; then
    (>&2 echo "Usage: $0 <P4 file> [-v P4VSN]")
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

shift


if [ "$PDB" != "" ]; then
    # note: Python 3.2+ also accepts a  -c continue  option
    # to remain compatible with Python 2.x, a "c" has to be typed at the start
    python -m "$PDB" src/compiler.py "${P4_SOURCE}" $*
else
    python src/compiler.py "${P4_SOURCE}" $*
fi

ERROR_CODE=$?
if [ "$ERROR_CODE" -ne 0 ]; then
    echo P4 compilation failed with error code $ERROR_CODE
    exit $ERROR_CODE
fi


mkdir -p "${P4DPDK_TARGET_DIR}"

cp -u makefiles/*.mk "${P4DPDK_TARGET_DIR}/"
if [ ! -f "${P4DPDK_TARGET_DIR}/Makefile" ]; then
    cat makefiles/Makefile | sed -e "s/example_dpdk1/${P4_BASENAME}/g" > "${P4DPDK_TARGET_DIR}/Makefile"
else
    if [[ "${@#--verbose}" != "$@" ]]; then
	    echo Makefile already exists, skipping
    fi
fi
