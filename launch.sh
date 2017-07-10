#!/bin/bash

print_usage_and_exit() {
    (>&2 echo "Usage: $0 <P4 file> [-v P4VSN] [controller name] [controller params file] -- <options for compiled switch>")
    exit 1
}

if [ $# -lt 1 ]; then
    print_usage_and_exit
fi

P4_SOURCE=$1
P4_BASENAME=$(basename ${P4_SOURCE%.*})

./compile.sh $*
if [ "$?" -ne 0 ]; then
    exit $?
fi

echo "-------------------- Starting execution"

shift

if [[ "$1" == "--p4v" || "$1" == "-v" ]]; then
    shift
    shift
fi

./run.sh ./build/$P4_BASENAME/build/$P4_BASENAME $*
