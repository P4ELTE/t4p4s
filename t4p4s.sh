#!/bin/bash

CONFIG_FILE=${CONFIG_FILE-examples.cfg}

ERROR_CODE=1

# bold and normal text
bb="\033[1;31m"
nn="\033[0m"

print_usage_and_exit() {
    (>&2 echo "Usage: $0 <options...> <P4 file>")
    (>&2 echo "- execution options: p4, c, run, launch, dbg (default: launch)")
    (>&2 echo "- further options:")
    (>&2 echo "    - v14|v16")
    (>&2 echo "    - cfg <T4P4S options>")
    (>&2 echo "    - ealopts <EAL options>")
    (>&2 echo "    - cmdopts <switch command line options>")
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

verbosemsg() {
    if [ -n "${T4P4S_VERBOSE+x}" ]; then
        msg "$@"
    fi
}

msg() {
    if [ "$T4P4S_SILENT" != 1 ]; then
        for var in "$@"
        do
            echo -e "$var"
        done
    fi
}

errmsg() {
    for var in "$@"
    do
        (>&2 echo -e "$var")
    done
}

errmsg_exit() {
    errmsg "Error: $*"
    print_usage_and_exit
}

exit_on_error() {
    ERROR_CODE=$?
    if [ "$ERROR_CODE" -ne 0 ]; then
        errmsg "Error: $1 (error code: $ERROR_CODE)"
        exit $ERROR_CODE
    fi
}

array_contains() {
    local n=$#
    local value=${!n}
    for ((i=1;i < $#;i++)) {
        if [ "${!i}" == "${value}" ]; then
            echo "y"
            return 0
        fi
    }
    echo "n"
    return 1
}

# Set defaults
ARCH=${ARCH-dpdk}
CTRL_PLANE_DIR=${CTRL_PLANE_DIR-./src/hardware_dep/shared/ctrl_plane}
PYTHON=${PYTHON-python}
T4P4S_VARIANT=${T4P4S_VARIANT-std}
CONTROLLER_OPTS=${CONTROLLER_OPTS-}

# Check if configuration is valid
[ "$ARCH" == "dpdk" ] && [ "${RTE_SDK}" == "" ] && errmsg_exit "Error: \$RTE_SDK not defined"
[ "${P4C}" == "" ] && errmsg_exit "Error: \$P4C not defined"


# Read configurations for the examples
declare -A t4p4s_opt_ids
declare -A single_variant
while read -r example variant opt_ids; do
    if [ "$example" == "" ]; then continue; fi

    opt="${example}__$variant"

    t4p4s_opt_ids[$opt]="$opt_ids"

    if [ "${single_variant[$example]}" != "-" ]; then
        if [ "${single_variant[$example]}" == "" ]; then
            single_variant[$example]="$variant"
        else
            single_variant[$example]="-"
        fi
    fi
done < <(grep -v ";" $CONFIG_FILE)


# Process arguments
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
                        OPTIDS="$1"
                        ;;
        "execopt")      shift
                        CMD_OPTS="$1"
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
            if [ "$T4P4S_EXAMPLE_TYPE" != "" ]; then
                msg "Example $T4P4S_EXAMPLE ($T4P4S_EXAMPLE_TYPE) already found, $bb$1$nn will override it"
            fi

            EXTENSION="${1##*.}"

            if [ "$EXTENSION" == "p4" ] || [ "$EXTENSION" == "p4_14" ]; then
                T4P4S_EXAMPLE_TYPE="p4"
            elif [ -x "$1" ]; then
                T4P4S_EXAMPLE_TYPE="executable"
            # there is exactly one corresponding file to the example name
            elif [ `find examples/ -type f -name "${1}.p4*" -printf '.' | wc -c` -eq 1 ]; then
                T4P4S_EXAMPLE_TYPE="example"
            fi

            if [ "$T4P4S_EXAMPLE_TYPE" == "" ]; then
                errmsg_exit "Cannot determine example configuration for parameter $bb$1$nn"
            fi

            T4P4S_EXAMPLE_PAR="$1"
            verbosemsg "Found $bb$T4P4S_EXAMPLE_PAR$nn ($T4P4S_EXAMPLE_TYPE)"
            ;;
    esac

    shift
done

[ "$T4P4S_EXAMPLE_TYPE" == "" ] && errmsg_exit "No example found"


# Sanity checks
[ "$T4P4S_P4" == 1 ] && [ "$T4P4S_EXAMPLE_TYPE" == "executable" ] && errmsg_exit "Executable $T4P4S_EXECUTABLE is already compiled"
[ "$T4P4S_C"  == 1 ] && [ "$T4P4S_EXAMPLE_TYPE" == "executable" ] && errmsg_exit "Executable $T4P4S_EXECUTABLE is already compiled"

# Default to launch if not specified
if [ "$T4P4S_P4" != 1 ] && [ "$T4P4S_C" != 1 ] && [ "$T4P4S_RUN" != 1 ]; then
    T4P4S_P4=1
    T4P4S_C=1
    T4P4S_RUN=1
    verbosemsg "Defaulting to launching ${EXAMPLE_SOURCE}"
fi


if [ "$T4P4S_EXAMPLE_TYPE" == "p4" ]; then
    EXAMPLE_SOURCE=${T4P4S_EXAMPLE_PAR}
    T4P4S_EXAMPLE_NAME=$(basename ${EXAMPLE_SOURCE%.*})
    T4P4S_CHOICE=${T4P4S_CHOICE-${T4P4S_EXAMPLE_NAME}__${T4P4S_VARIANT}}
    T4P4S_EXECUTABLE="./build/${T4P4S_CHOICE}/build/${T4P4S_EXAMPLE_NAME}"
elif [ "$T4P4S_EXAMPLE_TYPE" == "example" ]; then
    EXAMPLE_SOURCE=`find examples/ -type f -name "${T4P4S_EXAMPLE_PAR}.p4*"`
    T4P4S_EXAMPLE_NAME="${T4P4S_EXAMPLE_PAR}"
    T4P4S_CHOICE=${T4P4S_CHOICE-${T4P4S_EXAMPLE_NAME}__${T4P4S_VARIANT}}
    T4P4S_EXECUTABLE="./build/${T4P4S_CHOICE}/build/${T4P4S_EXAMPLE_NAME}"
elif [ "$T4P4S_EXAMPLE_TYPE" == "executable" ]; then
    T4P4S_EXAMPLE_NAME=$(basename "${T4P4S_EXAMPLE_PAR}")
    T4P4S_EXECUTABLE="${T4P4S_EXAMPLE_PAR}"
fi

OPTIDS=${OPTIDS-${t4p4s_opt_ids[$T4P4S_CHOICE]}}

# Read options with parameters
declare -A t4p4s_opts
for opt in $OPTIDS; do
    [[ $opt != *"="* ]] && continue

    IFS='=' read -r -a optparts <<< "$opt"
    t4p4s_opts[${optparts[0]}]="${optparts[1]}"
done


# Set defaults
CC_OPTS=""
CMD_OPTS=""
EAL_OPTS=""
CONTROLLER_OPTS=""

TESTDIR="examples"
TESTFILE="test_${T4P4S_EXAMPLE_NAME}_${T4P4S_TEST}.c"

T4P4S_TARGET_DIR=${T4P4S_TARGET_DIR-"./build/$T4P4S_CHOICE"}
T4P4S_SRCGEN_DIR=${T4P4S_SRCGEN_DIR-"./build/$T4P4S_CHOICE/srcgen"}
T4P4S_GEN_INCLUDE="${T4P4S_SRCGEN_DIR}/gen_include.h"

PYVSN="$($PYTHON -V 2>&1)"

GEN_MAKEFILE="${T4P4S_TARGET_DIR}/Makefile"

HUGEPAGES=${HUGEPAGES-${t4p4s_opts["hugepages"]}}
P4_VSN=${P4_VSN-${t4p4s_opts["vsn"]}}

[ "${T4P4S_SILENT}" == 1 ] && T4P4S_MAKE_OPTS="$T4P4S_MAKE_OPTS >/dev/null"

if [ -z "${P4_VSN+x}" ]; then
    P4_VSN=${p4vsn[$T4P4S_CHOICE]}
    [ "${P4_VSN}" !=  "" ] && verbosemsg "Using P4-${P4_VSN}, the default P4 version for the example ${T4P4S_EXAMPLE_NAME}"

    if [ "${P4_VSN}" == "" ]; then
        T4P4S_VARIANT="${single_variant[$T4P4S_EXAMPLE_NAME]}"
        [ $T4P4S_VARIANT ==  "" ] && errmsg_exit "No defaults in example.cfg for ${T4P4S_EXAMPLE_NAME} (variant ${T4P4S_VARIANT}) and none given"
        [ $T4P4S_VARIANT == "-" ] && errmsg_exit "No defaults in example.cfg for ${T4P4S_EXAMPLE_NAME} (variant ${T4P4S_VARIANT}) and none given"

        verbosemsg "Defaulting to variant ${T4P4S_VARIANT}, the only one in ${CONFIG_FILE} for ${T4P4S_EXAMPLE_NAME}"
    fi
fi

if [ -n "${T4P4S_DBGPY+x}" -a -z "${PDB+x}" ]; then
    PDB=${PDB-ipdb}

    verbosemsg "Using $PDB as debugger for $PYVSN"
fi


CONTROLLER_LOG=$(dirname $(dirname ${T4P4S_EXECUTABLE}))/controller.log


# Generate directories and files
mkdir -p $T4P4S_TARGET_DIR
mkdir -p $T4P4S_SRCGEN_DIR

rm -rf "${T4P4S_GEN_INCLUDE}"
echo "" >> "${T4P4S_GEN_INCLUDE}.tmp"

cat <<EOT >${GEN_MAKEFILE}
CDIR := \$(dir \$(lastword \$(MAKEFILE_LIST)))
APP = ${T4P4S_EXAMPLE_NAME}
include \$(CDIR)/../../makefiles/${ARCH}_backend_pre.mk
include \$(CDIR)/../../makefiles/common.mk
include \$(CDIR)/../../makefiles/hw_independent.mk
EOT

ARCH_OPTS_FILE=${ARCH_OPTS_FILE-t4p4s_${ARCH}.cfg}
# Default parameter sets
while read -r optid opttype optdata; do
    [ "$optid" == "" ] && continue

    [ "$opttype" == "ctrl" ] && sudo killall -q "$optid"

    [ $(array_contains ${OPTIDS[@]} "$optid") == "n" ] && continue

    [ "$opttype" == "cmdopts" ] && CMD_OPTS="$CMD_OPTS $optdata"
    [ "$opttype" == "ealopts" ] && EAL_OPTS="$EAL_OPTS $optdata"

    [ "$opttype" == "source"  ] && echo "SRCS-y += $optdata" >> "${GEN_MAKEFILE}"
    [ "$opttype" == "header"  ] && echo "#include \"$optdata\"" >> "${T4P4S_GEN_INCLUDE}.tmp"

    [ "$opttype" == "ctrl"    ] && CONTROLLER="$optdata"
    [ "$opttype" == "ctrcfg"  ] && CONTROLLER_OPTS="$CONTROLLER_OPTS $optdata"

    [ "$opttype" == "cflags"  ] && T4P4S_CFLAGS="$T4P4S_CFLAGS $optdata"
done < <(grep -v ";" "${ARCH_OPTS_FILE}")

# --------------------------------------------------------------------

reserve_hugepages() {
    HUGEPGSZ=`cat /proc/meminfo  | grep Hugepagesize | cut -d : -f 2 | tr -d ' '`
    OLD_HUGEPAGES=`cat /sys/kernel/mm/hugepages/hugepages-${HUGEPGSZ}/nr_hugepages`
    if [ $OLD_HUGEPAGES -lt $1 ]; then
        verbosemsg "Reserving $bb$1 hugepages$nn (previous size: $bb$OLD_HUGEPAGES$nn)"

        echo "echo $1 > /sys/kernel/mm/hugepages/hugepages-${HUGEPGSZ}/nr_hugepages" > .echo_tmp
        sudo sh .echo_tmp
        rm -f .echo_tmp
    else
        verbosemsg "Using $bb$OLD_HUGEPAGES hugepages$nn (sufficient, as $1 are needed)"
    fi
}

# $1.tmp is a file that has some (generated) contents; $1 may or may not exist
# Only (over)write $1 if the generated content differs from the existing one
overwrite_on_difference() {
    [ ! -f "$1" ] && cp "$1.tmp" "$1"
    cmp -s "$1.tmp" "$1"
    [ "$?" -ne 0 ] && mv "$1.tmp" "$1"
    rm -rf "$1.tmp"
}

# --------------------------------------------------------------------

# Phase 1: P4 to C compilation
if [ "$T4P4S_P4" == 1 ]; then
    msg "[${bb}COMPILE  P4-${P4_VSN}$nn] $bb${EXAMPLE_SOURCE}$nn@$bb${T4P4S_VARIANT}$nn"

    [ -n "${T4P4S_VERBOSE+x}" ] && export T4P4S_P4_OPTS="${T4P4S_P4_OPTS} -verbose"
    [ "$PDB" != "" ] && export T4P4S_P4_OPTS="${T4P4S_P4_OPTS} -m $PDB"

    $PYTHON src/compiler.py "${EXAMPLE_SOURCE}" --p4v $P4_VSN -g ${T4P4S_SRCGEN_DIR} ${T4P4S_P4_OPTS}
    exit_on_error "P4 to C compilation failed"
fi


# Phase 2: C compilation
if [ "$T4P4S_C" == 1 ]; then
    if [ -n "${T4P4S_TEST+x}" ]; then
        [ ! -f "$TESTDIR/$TESTFILE" ] && errmsg_exit "Test data file $bb$TESTFILE$nn in $bb$TESTDIR$nn not found"

        echo "VPATH += \$(CDIR)/../../$TESTDIR" >> "${GEN_MAKEFILE}"
        echo "SRCS-y += $TESTFILE" >> "${GEN_MAKEFILE}"
    fi

    overwrite_on_difference "${T4P4S_GEN_INCLUDE}"

    [ -n "${T4P4S_DBG+x}" ] && export T4P4S_CFLAGS="${T4P4S_CFLAGS} -DT4P4S_DEBUG"
    [ -n "${T4P4S_SILENT+x}" ] && export T4P4S_CFLAGS="${T4P4S_CFLAGS} -DT4P4S_SILENT"

    [ "${T4P4S_CFLAGS}" != "" ] && echo "CFLAGS += ${T4P4S_CFLAGS}" >> "${GEN_MAKEFILE}"
    echo "include \$(CDIR)/../../makefiles/${ARCH}_backend_post.mk" >> "${GEN_MAKEFILE}"

    msg "[${bb}COMPILE SWITCH$nn]"
    verbosemsg "Options for C compiler: ${bb}${T4P4S_CFLAGS}${nn}"

    cd ${T4P4S_TARGET_DIR}
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
    msg "[${bb}RUN CONTROLLER$nn]"

    msg "Controller     : $bb${CONTROLLER}$nn (default for $T4P4S_EXAMPLE_NAME)"
    verbosemsg "Controller log : $bb${CONTROLLER_LOG}$nn"
    verbosemsg "Controller opts: ${CONTROLLER_OPTS}"

    # Step 3A-1: Compile the controller
    cd $CTRL_PLANE_DIR
    if [ "${T4P4S_SILENT}" == 1 ]; then
        make -j $CONTROLLER >/dev/null
    else
        make -j $CONTROLLER
    fi
    exit_on_error "Controller compilation failed"
    cd - >/dev/null

    # Step 3A-3: Run controller
    stdbuf -o 0 $CTRL_PLANE_DIR/$CONTROLLER $CONTROLLER_OPTS > "${CONTROLLER_LOG}" &
    sleep 0.05
fi


# Phase 3B: Execution (switch)
if [ "$T4P4S_RUN" == 1 ]; then
    msg "[${bb}RUN SWITCH$nn]"

    sudo mkdir -p /mnt/huge

    grep -s '/mnt/huge' /proc/mounts > /dev/null
    if [ $? -ne 0 ] ; then
        sudo mount -t hugetlbfs nodev /mnt/huge
    fi

    [ "$HUGEPAGES" != "" ] && reserve_hugepages "$HUGEPAGES"

    [ "$ARCH" == "dpdk" ] && EXEC_OPTS="${EAL_OPTS} -- ${CMD_OPTS}"

    verbosemsg "Option ids : $bb${OPTIDS}$nn"
    verbosemsg "Options    : $bb${EXEC_OPTS}$nn"

    # Run T4P4S program
    if [ "${T4P4S_SILENT}" == 1 ]; then
        # TODO: is it possible to ignore the EAL's messages in a better way?
        sudo -E "${T4P4S_EXECUTABLE}" ${EXEC_OPTS} 2>&1 | egrep -v "^EAL: "
    else
        sudo -E "${T4P4S_EXECUTABLE}" ${EXEC_OPTS}
    fi
fi
