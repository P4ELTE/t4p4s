#!/bin/bash

EXAMPLE_FILE=examples.cfg

ERROR_CODE=1

function print_usage_and_exit {
    (>&2 echo "Usage: $0 <options...> <P4 file>")
    (>&2 echo "- execution options: p4, c, run, launch, dbg (default: launch)")
    (>&2 echo "- further options:")
    (>&2 echo "    - v14|v16")
    (>&2 echo "    - cfg <config string>")
    (>&2 echo "    - ctrl <controller name>")
    (>&2 echo "    - ctrcfg <controller config file>")
    (>&2 echo "    - var <variant name>")
    (>&2 echo "    - silent")
    (>&2 echo "    - dbg")
    (>&2 echo "    - dbgpy")
    exit $ERROR_CODE
}

function msg {
    if [ "$T4P4S_SILENT" != 1 ]; then
        for var in "$@"
        do
            echo "$var"
        done
    fi
}

function errmsg {
    for var in "$@"
    do
        (>&2 echo "$var")
    done
}

function errmsg_exit {
    errmsg "Error: $*"
    print_usage_and_exit
}

function exit_on_error {
    ERROR_CODE=$?
    if [ "$ERROR_CODE" -ne 0 ]; then
        errmsg "Error: $1 (error code: $ERROR_CODE)"
        exit $ERROR_CODE
    fi
}

if [ -z ${RTE_SDK+x} ]; then
    errmsg_exit "Error: \$RTE_SDK not defined"
fi

if [ -z ${P4C+x} ]; then
    errmsg_exit "Error: \$P4C not defined"
fi


# Controllers for the examples
declare -A p4vsn
declare -A dpdk_opt_ids
declare -A hugepages
while read -r example variant pvsn huges opt_ids; do
    if [ "$example" == "" ]; then continue; fi

    opt="${example}__$variant"

    p4vsn[$opt]="$pvsn"
    hugepages[$opt]="$huges"
    dpdk_opt_ids[$opt]="$opt_ids"
done < <(grep -v ";" $EXAMPLE_FILE)

# Default parameter sets
declare -A dpdk_gcc_opts
declare -A dpdk_sources
declare -A dpdk_controller
all_dpdk_controllers=()
declare -A dpdk_controller_params
declare -A dpdk_opts
while read -r opt gcc_opts sources controller controller_params opts; do
    if [ "$opt" == "" ]; then continue; fi

    dpdk_gcc_opts[$opt]="$gcc_opts"
    dpdk_sources[$opt]="$sources"
    dpdk_controller[$opt]="$controller"
    if [ "$controller" != "-" ]; then
        all_dpdk_controllers+=" $controller"
    fi
    dpdk_controller_params[$opt]="$controller_params"
    dpdk_opts[$opt]="$opts"
done < <(grep -v ";" dpdk_parameters.cfg)


# Setting defaults
ARCH=${ARCH-dpdk}
CTRL_PLANE_DIR=${CTRL_PLANE_DIR-./src/hardware_dep/shared/ctrl_plane}
PYTHON=${PYTHON-python}
T4P4S_VARIANT=${T4P4S_VARIANT-default}

# Processing arguments
while [ $# -gt 0 ]; do
    case "$1" in
        "v14")          P4_VSN=14
                        ;;
        "v16")          P4_VSN=16
                        ;;
        "p4")           T4P4S_P4=1
                        ;;
        "c")            T4P4S_C=1
                        ;;
        "run")          T4P4S_RUN=1
                        ;;
        "dbg")          T4P4S_P4=1
                        T4P4S_C=1
                        T4P4S_DBG=1
                        T4P4S_RUN=1
                        ;;
        "dbgpy")        T4P4S_P4=1
                        T4P4S_C=1
                        T4P4S_DBGPY=1
                        T4P4S_RUN=1
                        ;;
        "launch")       T4P4S_P4=1
                        T4P4S_C=1
                        T4P4S_RUN=1
                        ;;
        "silent")       T4P4S_SILENT=1
                        ;;
        "ctrl")         shift
                        T4P4S_CONTROLLER="$1"
                        ;;
        "cfg")          shift
                        DPDK_OPTS="$1"
                        ;;
        "ctrcfg")       shift
                        CONTROLLER_PARAMS_FILE="$1"
                        ;;
        "var")          shift
                        T4P4S_VARIANT="$1"
                        ;;
        *)
            if [ ! -z ${P4_SOURCE+x} ]; then
                errmsg_exit "Extra parameter: $1" "P4 source file already defined: ${P4_SOURCE}"
            elif [ ! -z ${P4_EXECUTABLE+x}  ]; then
                errmsg_exit "Extra parameter: $1" "P4 executable file already defined: ${P4_EXECUTABLE}"
            elif [ "${1##*.}" == "p4" ] || [ "${1##*.}" == "p4_16" ]; then
                P4_SOURCE=$1
                T4P4S_PRG=$(basename ${P4_SOURCE%.*})
                P4_EXECUTABLE="./build/${T4P4S_PRG}/build/${T4P4S_PRG}"
            else
                if [ ! -x "$1" ]; then
                    errmsg_exit "File $1 is not a P4 file or an executable"
                fi
                T4P4S_PRG=$(basename "$1")
                P4_EXECUTABLE="$1"
            fi
            ;;
    esac

    shift
done


P4DPDK_TARGET_DIR=${P4DPDK_TARGET_DIR-"./build/$T4P4S_PRG"}

T4P4S_CHOICE=${T4P4S_CHOICE-${T4P4S_PRG}__${T4P4S_VARIANT}}


# Sanity checks
if [ "$T4P4S_P4" != 1 ] && [ "$T4P4S_C" != 1 ] && [ "$T4P4S_RUN" != 1 ]; then
    if [ -n "${P4_SOURCE+x}" ]; then
        T4P4S_P4=1
        T4P4S_C=1
        T4P4S_RUN=1
        msg "Defaulting to launching ${P4_SOURCE}"
    else
        errmsg_exit "No action given, nothing to do"
    fi
fi

if [ -z "${P4_SOURCE+x}" ] && [ -z "${P4_EXECUTABLE+x}" ]; then
    errmsg_exit "No P4 file given"
fi


# Setting more defaults
if [ -z "${P4_VSN+x}" ]; then
    P4_VSN=${p4vsn[$T4P4S_CHOICE]}

    if [ "${P4_VSN}" == "" ]; then
        errmsg_exit "No defaults in example.cfg for ${T4P4S_PRG} (variant ${T4P4S_VARIANT}) and none given"
    fi

    msg "Using P4-${P4_VSN}, the default P4 version for the example ${T4P4S_PRG}"
fi

PYVSN="$($PYTHON -V 2>&1)"

if [ -n "${T4P4S_DBGPY+x}" -a -z "${PDB+x}" ]; then
    PDB=ipdb
    msg "Using $PDB as debugger for $PYVSN"
fi

if [ "${T4P4S_SILENT}" == 1 ]; then
    T4P4S_MAKE_OPTS="$T4P4S_MAKE_OPTS >/dev/null"
fi


if [ -z ${DPDK_OPTS+x} ]; then
    DPDK_OPTS=${dpdk_opt_ids[$T4P4S_CHOICE]}
fi

DPDK_OPTS_TEXT=""
for optid in $DPDK_OPTS; do
    if [ "${dpdk_gcc_opts[$optid]}" != "-" ]; then
        export P4_GCC_OPTS="${P4_GCC_OPTS} ${dpdk_gcc_opts[$optid]}"
    fi

    if [ "${dpdk_opts[$optid]}" != "-" ]; then
        DPDK_OPTS_TEXT="$DPDK_OPTS_TEXT ${dpdk_opts[$optid]}"
    fi
done




mkdir -p $P4DPDK_TARGET_DIR

# Phase 1: P4 to C compilation
if [ "$T4P4S_P4" == 1 ]; then
    msg "-------------------- Compiling P4-${P4_VSN} -> C"

    if [ "$PDB" != "" ]; then
        # note: Python 3.2+ also accepts a  -c continue  option
        # to remain compatible with Python 2.x, a "c" has to be typed at the start
        if echo "$PYVSN" | grep "Python 3[.][23456789]" >/dev/null; then
            echo $PYTHON -m "$PDB" -c continue src/compiler.py "${P4_SOURCE}" --p4v $P4_VSN
            $PYTHON -c continue -m "$PDB" src/compiler.py "${P4_SOURCE}" --p4v $P4_VSN
        else
            $PYTHON -m "$PDB" src/compiler.py "${P4_SOURCE}" --p4v $P4_VSN
        fi
    else
        $PYTHON src/compiler.py "${P4_SOURCE}" --p4v $P4_VSN
    fi

    exit_on_error "P4 to C compilation failed"
fi


# Phase 2: C compilation
if [ "$T4P4S_C" == 1 ]; then
    # Copying Makefiles
    rm -f ${P4DPDK_TARGET_DIR}/dpdk_backend.mk
    cp -u makefiles/*.mk "${P4DPDK_TARGET_DIR}/"

    for optid in $DPDK_OPTS; do
        if [ "${dpdk_sources[$optid]}" != "-" ]; then
            for source in $(echo "${dpdk_sources[$optid]}" | sed "s/,/ /g"); do
                echo "SRCS-y += $source" >> ${P4DPDK_TARGET_DIR}/dpdk_backend.mk
            done
        fi
    done

    if [ ! -f "${P4DPDK_TARGET_DIR}/Makefile" ]; then
        cat makefiles/Makefile | sed -e "s/example_dpdk1/${T4P4S_PRG}/g" > "${P4DPDK_TARGET_DIR}/Makefile"
    else
        if [[ "${@#--verbose}" != "$@" ]]; then
            echo Makefile already exists, skipping
        fi
    fi


    if [ -n "${T4P4S_DBG+x}" ]; then
        export P4_GCC_OPTS="${P4_GCC_OPTS} -DP4DPDK_DEBUG"
    fi

    if [ -n "${T4P4S_SILENT+x}" ]; then
        export P4_GCC_OPTS="${P4_GCC_OPTS} -DP4DPDK_SILENT"
    fi

    msg "-------------------- Compiling C -> switch executable"
    if [ ! -z ${P4_GCC_OPTS+x} ]; then
        msg "Options for C compiler: ${P4_GCC_OPTS}"
    fi

    cd ./build/$T4P4S_PRG
    if [ "${T4P4S_SILENT}" == 1 ]; then
        make -j >/dev/null
    else
        make -j
    fi
    exit_on_error "C compilation failed"

    cd - >/dev/null
fi


# Phase 3A: Execution (controller)
if [ "$T4P4S_RUN" == 1 ]; then
    msg "-------------------- Running controller"

    # Getting default parameters
    if [ -n ${CONTROLLER+x} ]; then
        for optid in $DPDK_OPTS; do
            if [ "${dpdk_controller[$optid]}" != "-" ]; then
                # TODO check for clashing controllers
                CONTROLLER=${dpdk_controller[$optid]}
                CONTROLLER_LOG=$(dirname $(dirname ${P4_EXECUTABLE}))/controller.log
            fi
        done

        if [ "$CONTROLLER" = "" ]; then
            errmsg_exit "No default controller found for $T4P4S_PRG"
        fi

        msg "Controller    : ${CONTROLLER} (default for $T4P4S_PRG)"
        msg "Controller log: ${CONTROLLER_LOG}"
    else
        CONTROLLER_LOG=$(dirname $(dirname ${P4_EXECUTABLE}))/controller.log
    fi

    # Step 3A-1: Compile the controller
    cd $CTRL_PLANE_DIR
    if [ "${T4P4S_SILENT}" == 1 ]; then
        make -j $CONTROLLER >/dev/null
    else
        make -j $CONTROLLER
    fi
    exit_on_error "Controller compilation failed"
    cd - >/dev/null

    # Step 3A-2: Stop all running controllers
    for controller in $all_dpdk_controllers; do
        sudo killall -q "$controller"
    done

    # Step 3A-3: Run controller
    stdbuf -o 0 $CTRL_PLANE_DIR/$CONTROLLER $CONTROLLER_PARAMS_FILE > "${CONTROLLER_LOG}" &
    sleep 0.05
fi


# Phase 3B: Execution (switch)
if [ "$T4P4S_RUN" == 1 ]; then
    msg "-------------------- Running switch"

    sudo mkdir -p /mnt/huge

    grep -s '/mnt/huge' /proc/mounts > /dev/null
    if [ $? -ne 0 ] ; then
            sudo mount -t hugetlbfs nodev /mnt/huge
    fi

    HUGEPGSZ=`cat /proc/meminfo  | grep Hugepagesize | cut -d : -f 2 | tr -d ' '`
    HUGEPAGES=${hugepages["$T4P4S_CHOICE"]}
    OLD_HUGEPAGES=`cat /sys/kernel/mm/hugepages/hugepages-${HUGEPGSZ}/nr_hugepages`
    if [ $OLD_HUGEPAGES -lt $HUGEPAGES ]; then
        msg "Reserving $HUGEPAGES hugepages (previous size: $OLD_HUGEPAGES)"

        msg "echo $HUGEPAGES > /sys/kernel/mm/hugepages/hugepages-${HUGEPGSZ}/nr_hugepages" > .echo_tmp
        sudo sh .echo_tmp
        rm -f .echo_tmp
    else
        msg "Using $OLD_HUGEPAGES hugepages (sufficient, as $HUGEPAGES are needed)"
    fi

    msg "DPDK options  : ${DPDK_OPTS}"
    msg "DPDK params   : ${DPDK_OPTS_TEXT}"

    # Run T4P4S program
    if [ "${T4P4S_SILENT}" == 1 ]; then
        # TODO: is it possible to ignore the EAL's messages in a better way?
        sudo -E "${P4_EXECUTABLE}" ${DPDK_OPTS_TEXT} 2>&1 | egrep -v "^EAL: "
    else
        sudo -E "${P4_EXECUTABLE}" ${DPDK_OPTS_TEXT}
    fi
fi
