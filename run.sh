#!/bin/bash

# controllers for the examples
declare -A controller_for_example
controller_for_example["l2_switch_test"]="l2_controller"
controller_for_example["l3_routing_test"]="l3_controller"

print_usage_and_exit() {
    (>&2 echo "Usage: $0 <switch executable> [controller name] [controller params file] -- <options for compiled switch>")
    exit 1
}

if [ $# -lt 1 ]; then
    (>&2 echo "Error: missing switch executable")
    print_usage_and_exit
fi

P4_SWITCH=$1
shift

if [ ! -f "$P4_SWITCH" ]; then
    (>&2 echo "Error: executable $P4_SWITCH does not exist")
    print_usage_and_exit
fi

P4_SWITCH_BASENAME=${P4_SWITCH##*/}
CONTROLLER=${controller_for_example["$P4_SWITCH_BASENAME"]}
if [ $# -gt 0 ]; then
    if [ "$1" = "default" ]; then
        shift
    elif [ "$1" != "--" ]; then
        CONTROLLER=$1
        shift
    fi
fi

if [ "$CONTROLLER" = "" -o "$CONTROLLER" = "--" ]; then
    (>&2 echo "Error: no default controller found for ${P4_SWITCH_BASENAME}")
    print_usage_and_exit
fi

CONTROLLER_PARAMS_FILE=""
if [ $# -ge 0 -a "$1" != "--" ]; then
    CONTROLLER_PARAMS_FILE="$1"
    shift
fi

# Getting arguments for switch from command line if needed
if [ -z ${P4_SWITCH_OPTS+x} ]; then
    while [ $# -gt 0 ]; do
        if [ "$1" = "--" ]; then
            shift
            P4_SWITCH_EXEC_OPTS=$*
            break
        fi

        shift

        if [ $# -eq 0 ]; then
            (>&2 echo "Error: no options were given for the compiled switch and \$P4_SWITCH_OPTS is not set")
            print_usage_and_exit
        fi
    done
else
    echo "Using switch options set in \$P4_SWITCH_OPTS"
    P4_SWITCH_EXEC_OPTS=${P4_SWITCH_OPTS}
fi

# Compile the controller
CTRL_PLANE_DIR=src/hardware_dep/shared/ctrl_plane
CONTROLLER_BUILD_DIR=build/controller

mkdir -p $CONTROLLER_BUILD_DIR
if [ ! -f "${CONTROLLER_BUILD_DIR}/Makefile" ]; then
    cp makefiles/controller/Makefile "${CONTROLLER_BUILD_DIR}/Makefile"
fi
cd $CONTROLLER_BUILD_DIR

make -j $CONTROLLER
ERROR_CODE=$?
if [ "$ERROR_CODE" -ne 0 ]; then
    echo Controller compilation failed with error code $ERROR_CODE
    exit 1
fi

cd - >/dev/null

# Stop all running controllers
for controller in "${controller_for_example[@]}"
do
    killall -q "$controller"
done

# Run controller
CONTROLLER_LOG=$(dirname $(dirname ${P4_SWITCH}))/controller.log
stdbuf -o 0 $CONTROLLER_BUILD_DIR/$CONTROLLER $CONTROLLER_PARAMS_FILE > "${CONTROLLER_LOG}" &
sleep 0.05

echo "-------------------- Running switch"
echo "Switch parameters  : ${P4_SWITCH_EXEC_OPTS}"
echo "Controller         : ${CONTROLLER}"
echo "Controller log file: ${CONTROLLER_LOG}"

# Start the program
sudo -E ${P4_SWITCH} ${P4_SWITCH_EXEC_OPTS}
