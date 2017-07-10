#!/bin/bash

print_usage_and_exit() {
    (>&2 echo "Usage: $0 <P4 file> [-v P4VSN]")
    exit 1
}

if [ $# -lt 1 ]; then
    print_usage_and_exit
fi

P4_SOURCE=$1
P4_BASENAME=$(basename ${P4_SOURCE%.*})
shift

if [ ! -f "$P4_SOURCE" ]; then
    (>&2 echo "Error: file not found: ${P4_SOURCE}")
    print_usage_and_exit
fi

if [[ "$1" == "--p4v" || "$1" == "-v" ]]; then
    shift
    P4_VSN_NUMBER="$1"
    P4_VSN="--p4v $1"
    shift
fi

if [ "$P4_VSN_NUMBER" == "" ]; then
    echo "-------------------- Compilation P4 -> C"
else
    echo "-------------------- Compilation P4-${P4_VSN_NUMBER} -> C"
fi

./compile_p4.sh $P4_SOURCE $P4_VSN
if [ "$?" -ne 0 ]; then
    exit 1
fi

echo "-------------------- Compilation C -> executable"

./compile_c.sh $P4_SOURCE $P4_VSN
if [ "$?" -ne 0 ]; then
    echo C compilation failed with error code $ERROR_CODE
    exit 1
fi
