#!/bin/bash

EXAMPLE_FILE=examples.cfg

ERROR_CODE=1

# bold and normal text
bb="\033[1m"
nn="\033[0m"

function print_usage_and_exit {
    (>&2 echo "Usage: $0 <options...> <P4 file>")
    (>&2 echo "- execution options: p4, c, run, launch, dbg (default: launch)")
    (>&2 echo "- further options:")
    (>&2 echo "    - v14|v16")
    (>&2 echo "    - cfg <T4P4S options>")
    (>&2 echo "    - dpdkcfg <DPDK options>")
    (>&2 echo "    - ctrl <controller name>")
    (>&2 echo "    - ctrcfg <controller config file>")
    (>&2 echo "    - var <variant name>")
    (>&2 echo "    - test <test case name>")
    (>&2 echo "    - silent")
    (>&2 echo "    - dbg")
    (>&2 echo "    - dbgpy")
    (>&2 echo "    - verbose")
    exit $ERROR_CODE
}

function verbosemsg {
    if [ -n "${T4P4S_VERBOSE+x}" ]; then
        msg "$@"
    fi
}

function msg {
    if [ "$T4P4S_SILENT" != 1 ]; then
        for var in "$@"
        do
            echo -e "$var"
        done
    fi
}

function errmsg {
    for var in "$@"
    do
        (>&2 echo -e "$var")
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
declare -A single_variant
while read -r example variant pvsn huges opt_ids; do
    if [ "$example" == "" ]; then continue; fi

    opt="${example}__$variant"

    p4vsn[$opt]="$pvsn"
    hugepages[$opt]="$huges"
    dpdk_opt_ids[$opt]="$opt_ids"

    if [ "${single_variant[$example]}" != "-" ]; then
        if [ "${single_variant[$example]}" == "" ]; then
            single_variant[$example]="$variant"
        else
            single_variant[$example]="-"
        fi
    fi
done < <(grep -v ";" $EXAMPLE_FILE)

# Default parameter sets
declare -A dpdk_gcc_opts
declare -A dpdk_headers
declare -A dpdk_sources
declare -A dpdk_controller
all_dpdk_controllers=()
declare -A dpdk_controller_opts
declare -A dpdk_opts
while read -r opt gcc_opts headers sources controller controller_opts opts; do
    if [ "$opt" == "" ]; then continue; fi

    dpdk_gcc_opts[$opt]="$gcc_opts"
    dpdk_sources[$opt]="$sources"
    dpdk_headers[$opt]="$headers"
    dpdk_controller[$opt]="$controller"
    if [ "$controller" != "-" ]; then
        all_dpdk_controllers+=" $controller"
    fi
    dpdk_controller_opts[$opt]="$controller_opts"
    dpdk_opts[$opt]="$opts"
done < <(grep -v ";" dpdk_parameters.cfg)


# Setting defaults
ARCH=${ARCH-dpdk}
CTRL_PLANE_DIR=${CTRL_PLANE_DIR-./src/hardware_dep/shared/ctrl_plane}
PYTHON=${PYTHON-python}
T4P4S_VARIANT=${T4P4S_VARIANT-std}
CONTROLLER_OPTS=${CONTROLLER_OPTS-}

# Processing arguments
while [ $# -gt 0 ]; do
    if [[ "$1" == \?\?* ]]; then
        T4P4S_TEST="${1#\?\?}"
        T4P4S_VARIANT="test"

        T4P4S_P4=1
        T4P4S_C=1
        T4P4S_DBG=1
        T4P4S_RUN=1

        shift

        continue
    fi

    if [[ "$1" == \?* ]]; then
        T4P4S_TEST="${1#\?}"
        T4P4S_VARIANT="test"
        shift

        continue
    fi

    if [[ "$1" == \@* ]]; then
        T4P4S_VARIANT="${1#\@}"
        shift

        continue
    fi

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
        "verbose")      T4P4S_VERBOSE="1"
                        ;;
        "ctrl")         shift
                        T4P4S_CONTROLLER="$1"
                        ;;
        "cfg")          shift
                        DPDK_OPTS="$1"
                        ;;
        "dpdkcfg")      shift
                        DPDK_OPTS_TEXT="$1"
                        ;;
        "ctrcfg")       shift
                        CONTROLLER_OPTS="$CONTROLLER_OPTS $1"
                        ;;
        "var")          shift
                        T4P4S_VARIANT="$1"
                        ;;
        "test")         shift
                        T4P4S_TEST="$1"
                        ;;
        *)
            if [ ! -z ${P4_SOURCE+x} ]; then
                errmsg_exit "Extra parameter: $1" "P4 source file already defined: $bb${P4_SOURCE}$nn"
            elif [ ! -z ${P4_EXECUTABLE+x}  ]; then
                errmsg_exit "Extra parameter: $1" "P4 executable file already defined: $bb${P4_EXECUTABLE}$nn"
            elif [ "${1##*.}" == "p4" ] || [ "${1##*.}" == "p4_16" ]; then
                P4_SOURCE=$1
                T4P4S_PRG=$(basename ${P4_SOURCE%.*})
                P4_EXECUTABLE="./build/${T4P4S_PRG}/build/${T4P4S_PRG}"
            else
                # this is the name of an example with a single variant, and there is exactly one corresponding file
                if [ "${single_variant[$1]}" != "" ] && [ `find examples/ -type f -name "${1}.p4*" -printf '.' | wc -c` -eq 1 ]; then
                    P4_SOURCE=`find examples/ -type f -name "${1}.p4*"`
                    T4P4S_PRG=$1
                    P4_EXECUTABLE="./build/$1/build/$1"
                else
                    if [ ! -x "$1" ]; then
                        errmsg_exit "File $bb$1$nn is not a P4 file or an executable"
                    fi

                    T4P4S_PRG=$(basename "$1")
                    P4_EXECUTABLE="$1"
                fi
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
        verbosemsg "Defaulting to launching ${P4_SOURCE}"
    else
        errmsg_exit "No action given, nothing to do"
    fi
fi

if [ -z "${P4_SOURCE+x}" ] && [ -z "${P4_EXECUTABLE+x}" ]; then
    errmsg_exit "No P4 file given"
fi


# Setting more defaults
TESTDIR="examples/p4_16_v1model/test"
TESTFILE="test_${T4P4S_PRG}_${T4P4S_TEST}.c"

if [ -z "${P4_VSN+x}" ]; then
    P4_VSN=${p4vsn[$T4P4S_CHOICE]}

    if [ "${P4_VSN}" == "" ]; then
        if [ "${single_variant[$T4P4S_PRG]}" != "" ] && [ "${single_variant[$T4P4S_PRG]}" != "-" ]; then
            T4P4S_VARIANT="${single_variant[$T4P4S_PRG]}"

            T4P4S_CHOICE="${T4P4S_PRG}__${T4P4S_VARIANT}"
            P4_VSN=${p4vsn[$T4P4S_CHOICE]}

            verbosemsg "Defaulting to variant ${T4P4S_VARIANT}, the only one in ${EXAMPLE_FILE} for ${T4P4S_PRG}"
        else
            errmsg_exit "No defaults in example.cfg for ${T4P4S_PRG} (variant ${T4P4S_VARIANT}) and none given"
        fi
    fi

    verbosemsg "Using P4-${P4_VSN}, the default P4 version for the example ${T4P4S_PRG}"
fi

PYVSN="$($PYTHON -V 2>&1)"

if [ -n "${T4P4S_DBGPY+x}" -a -z "${PDB+x}" ]; then
    PDB=ipdb

    verbosemsg "Using $PDB as debugger for $PYVSN"
fi

if [ "${T4P4S_SILENT}" == 1 ]; then
    T4P4S_MAKE_OPTS="$T4P4S_MAKE_OPTS >/dev/null"
fi


if [ -z ${DPDK_OPTS+x} ]; then
    DPDK_OPTS=${dpdk_opt_ids[$T4P4S_CHOICE]}
fi

if [ -z ${DPDK_OPTS_TEXT+x} ]; then
    DPDK_OPTS_TEXT=""
    for optid in $DPDK_OPTS; do
        if [ "${dpdk_gcc_opts[$optid]}" != "-" ]; then
            export P4_GCC_OPTS="${P4_GCC_OPTS} ${dpdk_gcc_opts[$optid]}"
        fi

        if [ "${dpdk_opts[$optid]}" != "-" ]; then
            DPDK_OPTS_TEXT="$DPDK_OPTS_TEXT ${dpdk_opts[$optid]}"
        fi
    done
fi



mkdir -p $P4DPDK_TARGET_DIR

# Phase 1: P4 to C compilation
if [ "$T4P4S_P4" == 1 ]; then
    msg "[${bb}P4-${P4_VSN}  -> C$nn] $bb${P4_SOURCE}$nn@$bb${T4P4S_VARIANT}$nn"

    if [ -n "${T4P4S_VERBOSE+x}" ]; then
        export T4P4S_P4_OPTS="${T4P4S_P4_OPTS} -verbose"
    fi

    if [ "$PDB" != "" ]; then
        # note: Python 3.2+ also accepts a  -c continue  option
        # to remain compatible with Python 2.x, a "c" has to be typed at the start
        if echo "$PYVSN" | grep "Python 3[.][23456789]" >/dev/null; then
            $PYTHON -c continue -m "$PDB" src/compiler.py "${P4_SOURCE}" --p4v $P4_VSN ${T4P4S_P4_OPTS}
        else
            $PYTHON -m "$PDB" src/compiler.py "${P4_SOURCE}" --p4v $P4_VSN ${T4P4S_P4_OPTS}
        fi
    else
        $PYTHON src/compiler.py "${P4_SOURCE}" --p4v $P4_VSN ${T4P4S_P4_OPTS}
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

    for optid in $DPDK_OPTS; do
        if [ "${dpdk_headers[$optid]}" != "-" ]; then
            P4DPDK_GEN_INCLUDE="${P4DPDK_TARGET_DIR}/../src_hardware_indep/dpdk_gen_include.h"
            rm -rf "${P4DPDK_GEN_INCLUDE}"

            echo "" >> "${P4DPDK_GEN_INCLUDE}"

            for header in $(echo "${dpdk_headers[$optid]}" | sed "s/,/ /g"); do
                echo "#include \"${header}\"" >> "${P4DPDK_GEN_INCLUDE}"
            done
        fi
    done



    if [ ! -f "${P4DPDK_TARGET_DIR}/Makefile" ]; then
        cat makefiles/Makefile | sed -e "s/example_dpdk1/${T4P4S_PRG}/g" > "${P4DPDK_TARGET_DIR}/Makefile"
    else
        if [[ "${@#--verbose}" != "$@" ]]; then
            verbosemsg Makefile already exists, skipping
        fi
    fi


    if [ -n "${T4P4S_TEST+x}" ]; then
        echo "VPATH += \$(P4_SRCDIR)/../$TESTDIR" >> "${P4DPDK_TARGET_DIR}/dpdk_backend.mk"

        if [ -f "$TESTDIR/$TESTFILE" ]; then
            echo "SRCS-y += $TESTFILE" >> ${P4DPDK_TARGET_DIR}/dpdk_backend.mk
        else
            errmsg_exit "Test data file $bb$TESTFILE$nn in $bb$TESTDIR$nn not found"
        fi
    fi


    if [ -n "${T4P4S_DBG+x}" ]; then
        export P4_GCC_OPTS="${P4_GCC_OPTS} -DP4DPDK_DEBUG"
    fi

    if [ -n "${T4P4S_SILENT+x}" ]; then
        export P4_GCC_OPTS="${P4_GCC_OPTS} -DP4DPDK_SILENT"
    fi

    msg "[${bb}C$nn -> ${bb}switch$nn]"
    if [ ! -z ${P4_GCC_OPTS+x} ]; then
        msg "Options for C compiler: ${bb}${P4_GCC_OPTS}${nn}"
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

        msg "Controller     : $bb${CONTROLLER}$nn (default for $T4P4S_PRG)"
        verbosemsg "Controller log : $bb${CONTROLLER_LOG}$nn"
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


    if [ "$CONTROLLER_OPTS" == "" ]; then
        for optid in $DPDK_OPTS; do
            if [ "${dpdk_controller_opts[$optid]}" != "-" ]; then
                for controller_opt in "${dpdk_controller_opts[$optid]}"; do
                    CONTROLLER_OPTS="$CONTROLLER_OPTS $controller_opt"
                done
            fi
        done
    fi

    verbosemsg "Controller opts: ${CONTROLLER_OPTS}"

    # Step 3A-3: Run controller
    stdbuf -o 0 $CTRL_PLANE_DIR/$CONTROLLER $CONTROLLER_OPTS > "${CONTROLLER_LOG}" &
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
        verbosemsg "Reserving $bb$HUGEPAGES hugepages$nn (previous size: $bb$OLD_HUGEPAGES$nn)"

        echo "echo $HUGEPAGES > /sys/kernel/mm/hugepages/hugepages-${HUGEPGSZ}/nr_hugepages" > .echo_tmp
        sudo sh .echo_tmp
        rm -f .echo_tmp
    else
        verbosemsg "Using $bb$OLD_HUGEPAGES hugepages$nn (sufficient, as $HUGEPAGES are needed)"
    fi

    verbosemsg "DPDK options  : $bb${DPDK_OPTS}$nn"
    verbosemsg "DPDK params   : $bb${DPDK_OPTS_TEXT}$nn"

    # Run T4P4S program
    if [ "${T4P4S_SILENT}" == 1 ]; then
        # TODO: is it possible to ignore the EAL's messages in a better way?
        sudo -E "${P4_EXECUTABLE}" ${DPDK_OPTS_TEXT} 2>&1 | egrep -v "^EAL: "
    else
        sudo -E "${P4_EXECUTABLE}" ${DPDK_OPTS_TEXT}
    fi
fi
