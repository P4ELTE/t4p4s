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
P4_BUILD_DIR=${P4_BUILD_DIR-"build/$P4_BASENAME"}

python2 src/compiler.py "${P4_SOURCE}" "${P4_BUILD_DIR}/src_hardware_indep"
ERROR_CODE=$?
if [ "$ERROR_CODE" -ne 0 ]; then
    echo P4 compilation failed with error code $ERROR_CODE
    exit 1
fi

cp -u makefiles/switch/*.mk "${P4_BUILD_DIR}/"
if [ ! -f "${P4_BUILD_DIR}/Makefile" ]; then
    cat makefiles/switch/Makefile | sed -e "s/placeholder_name/${P4_BASENAME}/g" > "${P4_BUILD_DIR}/Makefile"
fi
