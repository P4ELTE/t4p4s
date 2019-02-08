#!/bin/bash

# --------------------------------------------------------------------
# Set defaults

ERROR_CODE=1

COLOURS_CONFIG_FILE=${COLOURS_CONFIG_FILE-colours.cfg}
LIGHTS_CONFIG_FILE=${LIGHTS_CONFIG_FILE-lights.cfg}
EXAMPLES_CONFIG_FILE=${EXAMPLES_CONFIG_FILE-examples.cfg}

P4_SRC_DIR=${P4_SRC_DIR-"./examples/"}
CTRL_PLANE_DIR=${CTRL_PLANE_DIR-./src/hardware_dep/shared/ctrl_plane}

ARCH=${ARCH-dpdk}
ARCH_OPTS_FILE=${ARCH_OPTS_FILE-opts_${ARCH}.cfg}

PYTHON=${PYTHON-python}
DEBUGGER=${DEBUGGER-gdb}

declare -A EXT_TO_VSN=(["p4"]=16 ["p4_14"]=14)
ALL_COLOUR_NAMES=(action bytes control core default error expected extern field header headertype off packet parserstate port smem socket status success table testcase warning)

# --------------------------------------------------------------------
# Helpers

exit_program() {
    echo -e "$nn"
    [ "${OPTS[ctr]}" != "" ] && verbosemsg "Terminating controller $(cc 0)dpdk_${OPTS[ctr]}_controller$nn" && sudo killall -q "dpdk_${OPTS[ctr]}_controller"
    exit $ERROR_CODE
}

print_usage_and_exit() {
    (>&2 echo -e "Usage: $0 <options...>")
    (>&2 echo -e "    - see $(cc 0)README.md$nn for details")

    exit_program $ERROR_CODE
}

verbosemsg() {
    [ "$(optvalue verbose)" != off ] && msg "$@"
    return 0
}

msg() {
    [ "$(optvalue silent)" != off ] && return

    for msgvar in "$@"; do
        echo -e "$msgvar"
    done
}

errmsg() {
    for msgvar in "$@"; do
        (>&2 echo -e "$msgvar")
    done
}

errmsg_exit() {
    errmsg "Error: $*"
    print_usage_and_exit
}

exit_on_error() {
    ERROR_CODE=$?
    [ "$ERROR_CODE" -eq 0 ] && return

    errmsg "Error: $1 (error code: $ERROR_CODE)"
    exit_program $ERROR_CODE
}

array_contains() {
    local value=$1
    shift

    for ((i=1;i <= $#;i++)) {
        [ "${!i}" == "${value}" ] && echo "y" && return
    }
    echo "n"
}

# Return the first valid colour code in the args, or the neutral colour if no valid colour is found.
cc() {
    [ "$(array_contains "${OPTS[bw]}" "on" "terminal")" == y ] && echo "$nn" && return

    while [ $# -gt 0 ]; do
        [ "${colours[$1]}" != "" ] && echo "${colours[$1]}" && return
        shift
    done
    echo "$nn"
}

# Set lit terminal text to colour indicated by `$1`.
set_term_light() {
    OPTS[light]=$1
    colours=()

    [ "$1" == "0" ] && nn="" && return

    IFS=',' read -r -a optparts <<< "$1"
    for i in "${!optparts[@]}"; do 
        COLOUR=${optparts[$i]}
        COLOUR=${KNOWN_COLOURS[$COLOUR]-$COLOUR}

        colours[$i]="\033[${COLOUR}m"
    done
    nn="\033[0m"
}

remove_name_markers() {
    [[ "$1" == \?\?*     ]] && echo "${1#\?\?}" && return
    [[ "$1" == \?*       ]] && echo "${1#\?}"   && return
    [[ "$1" == \@*       ]] && echo "${1#\@}"   && return
    [[ "$1" == \**       ]] && echo "${1#\*}"   && return

    echo "$1"
}

print_cmd_opts() {
    IFS=' ' read -r -a cflags <<< "$1"

    NEXT_IS_OPT=0    
    for cflag in ${cflags[@]}; do
        [ $NEXT_IS_OPT -eq 1 ] && NEXT_IS_OPT=0 && echo "$(cc 1)${cflag}$nn" && continue

        IFS='=' read -r -a parts <<< "$cflag"

        KNOWN_OPT_FLAGS=(-g --p4v -U --log-level -c -n --config -p)

        [ "$(array_contains "${parts[0]}" "${KNOWN_OPT_FLAGS[@]}")" == y ] && NEXT_IS_OPT=1

        OPTTXT1=${parts[0]}
        OPTTXT2=""
        OPTTXT3=""
        OPTTXT4=""

        [[ "${parts[0]}" == -*  ]]   && OPTTXT1="-"  && OPTTXT3=${parts[0]##-}
        [[ "${parts[0]}" == -D* ]]   && OPTTXT1="-D" && OPTTXT3=${parts[0]##-D}
        [[ "${parts[0]}" == --* ]]   && OPTTXT1="--" && OPTTXT3=${parts[0]##--}
        [[ "${parts[0]}" == *.p4* ]] && OPTTXT1="${parts[0]%/*}/" && OPTTXT2="${parts[0]##*/}" && OPTTXT2="${OPTTXT2%%.p4*}" && OPTTXT4=".${parts[0]##*.}"

        echo "$(cc 0)$OPTTXT1$(cc 1)$OPTTXT2$nn$(cc 2)$OPTTXT3$nn${parts[1]+=$(cc 1)${parts[1]}}$(cc 2 0)$OPTTXT4$nn$nn"
    done | tr '\n' ' '
}

print_opts() {
    declare -A all_opts
    for k in "${!OPTS[@]}" "${!IGNORE_OPTS[@]}"; do
        all_opts[$k]=1
    done

    for optid in ${!all_opts[@]}; do
        [[ $optid = [A-Z]*_* ]] && continue

        PREFIX="$(cc 0)"
        [ "${OPTS[$optid]}" == "on" ] && PREFIX="$(cc 2)"
        [ "${IGNORE_OPTS[$optid]}" == "on" ] && PREFIX="$(cc 3 2)^"
        [ "${OPTS[$optid]}" != "" -a "${OPTS[$optid]}" != "on" ] && echo "$PREFIX$optid$nn=$(cc 1)${OPTS[$optid]}$nn" && continue
        echo "$PREFIX$optid$nn"
    done | sort | tr '\n' ', '
}

setopt() {
    [ "${OPTS["$1"]}" == "off" ] && echo -e "Option ${OPTS["$1"]} is set to be ignored" && return
    OPTS[$1]="$2"
}

# $1: the option name, $2: the option value, $3: separator
addopt() {
    OPTS[$1]="${OPTS[$1]:+${OPTS[$1]}$3}${2}"
}

optvalue() {
    [ "${IGNORE_OPTS["$1"]}" != "" ] && echo "off" && return
    [ "${OPTS[$1]}" == "" ] && echo "off" && return
    echo "${OPTS[$1]}"
}

# --------------------------------------------------------------------

# Unmounting /mnt/huge and removing directory
remove_mnt_huge()
{
    grep -s '/mnt/huge' /proc/mounts > /dev/null
    [ $? -eq 0 ]     && sudo umount /mnt/huge
    [ -d /mnt/huge ] && sudo rm -R /mnt/huge
}

# Creating /mnt/huge and mounting as hugetlbfs
create_mnt_huge()
{
    sudo mkdir -p /mnt/huge
    grep -s '/mnt/huge' /proc/mounts > /dev/null
    [ $? -ne 0 ] && sudo mount -t hugetlbfs nodev /mnt/huge
}

allocate_huge_pages() {
    echo "echo $2 > /sys/kernel/mm/hugepages/hugepages-$1/nr_hugepages" > .echo_tmp
    sudo sh .echo_tmp
    rm -f .echo_tmp
}

# Removing currently reserved hugepages
create_huge_pages()
{
    allocate_huge_pages $1 $2
    create_mnt_huge
}

# Removing currently reserved hugepages
clear_huge_pages()
{
    allocate_huge_pages $1 0
    remove_mnt_huge
}

reserve_hugepages() {
    HUGEPGSZ=`cat /proc/meminfo  | grep Hugepagesize | cut -d : -f 2 | tr -d ' '`
    OLD_HUGEPAGES=`cat /sys/kernel/mm/hugepages/hugepages-${HUGEPGSZ}/nr_hugepages`

    [ $OLD_HUGEPAGES -eq $1 ] && verbosemsg "Using $(cc 0)$OLD_HUGEPAGES$nn hugepages (sufficient, as $(cc 0)$1$nn are needed)" && return

    if [ $OLD_HUGEPAGES -lt $1 ]; then
        msg "Allocating $(cc 0)$1 hugepages$nn (previous size: $(cc 0)$OLD_HUGEPAGES$nn)"

        create_huge_pages $HUGEPGSZ $1
        return
    fi

    [ "$(optvalue keephuge)" != off ] && return

    msg "Allocating $(cc 0)$1$nn hugepages (clearing $(cc 0)$OLD_HUGEPAGES$nn old hugepages)"
    clear_huge_pages "$HUGEPGSZ"
    allocate_huge_pages "$HUGEPGSZ" "$1"
}

reserve_hugepages2() {
    HUGEPGSZ=`cat /proc/meminfo  | grep Hugepagesize | cut -d : -f 2 | tr -d ' '`
    OLD_HUGEPAGES=`cat /sys/kernel/mm/hugepages/hugepages-${HUGEPGSZ}/nr_hugepages`
    if [ $OLD_HUGEPAGES -lt ${OPTS[hugepages]} ]; then
        verbosemsg "Reserving $(cc 0)${OPTS[hugepages]} hugepages$nn (previous size: $(cc 0)$OLD_HUGEPAGES$nn)"

        echo "echo ${OPTS[hugepages]} > /sys/kernel/mm/hugepages/hugepages-${HUGEPGSZ}/nr_hugepages" > .echo_tmp
        sudo sh .echo_tmp
        rm -f .echo_tmp
    else
        verbosemsg "Using $(cc 0)$OLD_HUGEPAGES hugepages$nn (sufficient, as $(cc 0)${OPTS[hugepages]}$nn are needed)"
    fi
}

# /tmp/$1.tmp is a file that has some (generated) contents; $2/$1 may or may not exist
# Only (over)write $1 if the generated content differs from the existing one
overwrite_on_difference() {
    cmp -s "/tmp/$1.tmp" "$2/$1"
    [ "$?" -ne 0 ] && mv "/tmp/$1.tmp" "$2/$1"
    rm -f "/tmp/$1.tmp"
}

# --------------------------------------------------------------------
# Set defaults

declare -A KNOWN_COLOURS
declare -A OPTS
declare -A IGNORE_OPTS

colours=()
nn="\033[0m"

# Check if configuration is valid
[ "${P4C}" == "" ] && errmsg_exit "Error: \$P4C not defined"
[ "$ARCH" == "dpdk" ] && [ "${RTE_SDK}" == "" ] && errmsg_exit "Error: \$RTE_SDK not defined"

# --------------------------------------------------------------------
# Parse opts from files and command line

PYTHON_PARSE_OPT=$(cat <<END
import re
import sys

# To facilitate understanding, almost all named patterns of the regex are separated
patterns = (
    ("cond",      '!condvar(=!condval)?!condsep'),
    ("prefix",    '(\^|:|::|%|%%|@)'),
    ("condvar",   '[a-zA-Z0-9_\-.]+'),
    ("condval",   '[^\s].*'),
    ("condsep",   '(\s*->\s*)'),
    ("letop",     '\+{0,2}='),
    ("letval",    '[^\s].*'),
    ("var",       '[a-zA-Z0-9_\-.]+'),
    ("comment",   '\s*(;.*)?'),
    )

rexp = '^(!prefix|!cond?)?!var(?P<let>\s*!letop?\s*!letval)?!comment$'

# Assemble the full regex
for pattern, replacement in patterns:
    rexp = rexp.replace("!" + pattern, "(?P<{}>{})".format(pattern, replacement))

rexp = re.compile(rexp)

m = re.match(rexp, sys.argv[1])

print 'ok', ('y' if m is not None else 'n')
for gname in (p[0] for p in patterns):
    print gname, '' if m is None else m.group(gname) or ''
END
)


candidates() {
    if [ $(find "$P4_SRC_DIR" -type f -name "*$1*.p4*" | wc -l) -gt 0 ]; then
        echo
        find "$P4_SRC_DIR" -type f -name "*$1*.p4*" | sed 's#^.*/\([^\.]*\).*$#    \1#g'
    else
        echo "(no candidates found)"
    fi
}

OPT_NOPRINTS=("OPT_NOPRINTS" "cfgfiles")

CFGFILES=${CFGFILES-${COLOURS_CONFIG_FILE},${LIGHTS_CONFIG_FILE},!cmdline!,!varcfg!${EXAMPLES_CONFIG_FILE},${ARCH_OPTS_FILE}}
declare -A OPTS=([cfgfiles]="$CFGFILES")


while [ "${OPTS[cfgfiles]}" != "" ]; do
    IFS=',' read -r -a cfgfiles <<< "${OPTS[cfgfiles]}"
    OPTS[cfgfiles]=""

    for cfgfile in ${cfgfiles[@]}; do
        declare -a NEWOPTS=()

        # Collect option descriptions either from the command line or a file
        if [ "$cfgfile" == "!cmdline!" ]; then
            OPT_ORIGIN="$(cc 0)command line$nn"
            for opt; do
                NEWOPTS+=("$opt")
            done
        elif [[ $cfgfile =~ !varcfg!* ]]; then
            OPT_ORIGIN="$(cc 0)variant config file$nn $(cc 1)${cfgfile#!varcfg!}$nn"
            examplename="${OPTS[example]}@${OPTS[variant]}"
            [ "${OPTS[variant]}" == "std" ] && examplename="${OPTS[example]}\(@std\)\?"

            IFS=$'\n'
            while read -r opts; do
                IFS=' ' read -r -a optparts <<< "$opts"

                for opt in ${optparts[@]}; do
                    NEWOPTS+=("$opt")
                done
            done < <(cat "${cfgfile#!varcfg!}" | grep -e "^$examplename\s" | sed -e "s/^[^ \t]+[ \t]*//g")
        else
            OPT_ORIGIN="$(cc 0)file$nn $(cc 1)${cfgfile}$nn"
            IFS=$'\n'
            while read -r opt; do
                NEWOPTS+=("$opt")
            done < <(cat "${cfgfile}")
        fi

        verbosemsg "Parsing $OPT_ORIGIN"

        # Process the options
        for opt in ${NEWOPTS[@]}; do
            if [[ $opt == *.p4* ]] && [ -f "$opt" ]; then
                setopt example "$(basename ${opt%.*})"
                setopt source "$opt"
                continue
            fi

            # Split the option into its components along the above regex
            IFS=' '
            declare -A groups=() && while read -r grpid grptxt; do groups["$grpid"]="$grptxt"; done < <(python -c "$PYTHON_PARSE_OPT" "$opt")
            [ "${groups[ok]}" == n ] && [[ $opt = *\;* ]] && continue
            [ "${groups[ok]}" == n ] && echo -e "Cannot parse option $(cc 0)$opt$nn (origin: $OPT_ORIGIN)" && continue

            var="${groups[var]}"
            value="${groups[letval]:-on}"
            [ "${groups[neg]}" != "" ] && OPTS[$var]=off && continue

            if [ "${groups[cond]}" != "" ]; then
                expected_value="${groups[condval]}"
                [ "$(optvalue "${groups[condvar]}")" == off ] && continue
                [ "$expected_value" != "" -a "${OPTS[${groups[condvar]}]}" != "$expected_value" ] && continue
            fi

            [[ $var == COLOUR_* ]] && KNOWN_COLOURS[${var#COLOUR_}]="$value"
            [ "$var" == "light" ] && set_term_light "$value" && continue

            [ "$var" == cfgfiles -a ! -f "$value" ] && echo -e "Config file $(cc 0)$value$nn cannot be found" && continue

            if [ "$(array_contains "${groups[prefix]}" ":" "::" "%" "%%")" == y ]; then
                FIND_COUNT=`find "$P4_SRC_DIR" -type f -name "${var}.p4*" -printf '.' | wc -c`
                [ $FIND_COUNT -gt 1 ] && errmsg_exit "Name is not unique: found $(cc 1)$FIND_COUNT$nn P4 files for $(cc 0)${var}$nn"
                [ $FIND_COUNT -eq 0 ] && errmsg_exit "Could not find P4 file for $(cc 0)${var}$nn, candidates: $(cc 1)$(candidates ${var})$nn"

                setopt example "$var"
                setopt source "`find "$P4_SRC_DIR" -type f -name "${var}.p4*"`"
            fi

            [ "${groups[prefix]}" == ":"  ] && setopt example "$var" && continue
            [ "${groups[prefix]}" == "::" ] && setopt example "$var" && setopt dbg on && continue
            [ "${groups[prefix]}" == "%"  ] && [ "$value" == "on" ]  && verbosemsg "Test case not specified for example $(cc 0)$var$nn, using $(cc 1)test$nn as default" && value="test"
            [ "${groups[prefix]}" == "%"  ] && setopt example "$var" && setopt testcase "$value" && setopt variant test && continue
            [ "${groups[prefix]}" == "%%" ] && [ "$value" == "on" ] && setopt example "$var" && setopt suite on && setopt dbg on && setopt variant test && continue
            [ "${groups[prefix]}" == "%%" ] && setopt example "$var" && setopt testcase "$value" && setopt dbg on && setopt variant test && continue
            [ "${groups[prefix]}" == "@"  ] && setopt variant "$var" && continue
            [ "${groups[prefix]}" == "^"  ] && IGNORE_OPTS["$var"]=on && continue

            [ "${groups[letop]}" == "+="  ] && addopt "$var" "$value" " " && continue
            [ "${groups[letop]}" == "++=" ] && addopt "$var" "$value" "\n" && continue

            setopt "$var" "$value"
        done

        # Steps after processing the command line
        if [ "$cfgfile" == "!cmdline!" ]; then
            # The command line must specify an example to run
            [ "$(optvalue example)" == off ] && errmsg_exit "No example to run"
            # The variant has to be determined before processing the config files.
            [ "$(optvalue variant)" == off ] && setopt variant std
        fi
    done
done


[ "$(optvalue verbose)" == on ] && IGNORE_OPTS[silent]=on
[ "$(optvalue silent)" == on  ] && IGNORE_OPTS[verbose]=on


[ "$(optvalue variant)" == off ] && [ "$(optvalue testcase)" != off -o "$(optvalue suite)" != off ] && OPTS[variant]=test && verbosemsg "Variant $(cc 1)@test$nn is chosen because testing is requested"
[ "${OPTS[variant]}" == "" -o "${OPTS[variant]}" == "-" ] && OPTS[variant]=std && verbosemsg "Variant $(cc 1)@std$nn is automatically chosen"


# Determine version by extension if possible
if [ "${OPTS[vsn]}" == "" ]; then
    P4_EXT="$(basename "${OPTS[source]}")"
    P4_EXT=${P4_EXT##*.}
    if [ "$(array_contains "${P4_EXT##*.}" ${!EXT_TO_VSN[@]})" == n ]; then
        errmsg_exit "Cannot determine P4 version for the extension $(cc 0)${P4_EXT}$nn of $(cc 0)$(print_cmd_opts "${OPTS[source]}")$nn"
    fi
    OPTS[vsn]="${EXT_TO_VSN["${P4_EXT##*.}"]}"
    [ "${OPTS[vsn]}" == "" ] && errmsg_exit "Cannot determine P4 version for $(cc 0)${OPTS[example]}$nn"
    verbosemsg "Determined P4 version to be $(cc 0)${OPTS[vsn]}$nn by the extension of $(cc 0)$(print_cmd_opts "${OPTS[source]}")$nn"
fi

OPTS[choice]=${T4P4S_CHOICE-${OPTS[example]}@${OPTS[variant]}}
OPTS[executable]="./build/${OPTS[choice]}/build/${OPTS[example]}"

T4P4S_TARGET_DIR=${T4P4S_TARGET_DIR-"./build/${OPTS[choice]}"}
T4P4S_SRCGEN_DIR=${T4P4S_SRCGEN_DIR-"./build/${OPTS[choice]}/srcgen"}
T4P4S_GEN_INCLUDE_DIR="${T4P4S_SRCGEN_DIR}"
T4P4S_GEN_INCLUDE="gen_include.h"
GEN_MAKEFILE_DIR="${T4P4S_TARGET_DIR}"
GEN_MAKEFILE="Makefile"

T4P4S_LOG_DIR=${T4P4S_LOG_DIR-$(dirname $(dirname ${OPTS[executable]}))/log}

# By default use all three phases
if [ "${OPTS[p4]}" != on ] && [ "${OPTS[c]}" != on ] && [ "${OPTS[run]}" != on ]; then
    OPTS[p4]=on
    OPTS[c]=on
    OPTS[run]=on
fi

# Generate directories and files
mkdir -p $T4P4S_TARGET_DIR
mkdir -p $T4P4S_SRCGEN_DIR


[ "$(optvalue silent)" != off ] && addopt makeopts ">/dev/null" " "


# --------------------------------------------------------------------

verbosemsg "Options: $(print_opts)"

# Phase 0: If a phase with root access is needed, ask for it now
if [ "$(optvalue run)" != off ]; then
    verbosemsg "Requesting root access..."
    sudo echo -n ""
    verbosemsg "Root access granted, starting..."
fi

# Phase 1: P4 to C compilation
if [ "$(optvalue p4)" != off ]; then
    msg "[$(cc 0)COMPILE  P4-${OPTS[vsn]}$nn] $(cc 0)$(print_cmd_opts ${OPTS[source]})$nn@$(cc 1)${OPTS[variant]}$nn${OPTS[testcase]+, test case $(cc 1)${OPTS[testcase]-(none)}$nn}${OPTS[dbg]+, $(cc 0)debug$nn mode}"

    addopt p4opts "${OPTS[source]}" " "
    addopt p4opts "--p4v ${OPTS[vsn]}" " "
    addopt p4opts "-g ${T4P4S_SRCGEN_DIR}" " "
    [ "$(optvalue verbose)" != off ] && addopt p4opts "-verbose" " "

    verbosemsg "P4 compiler options: $(print_cmd_opts "${OPTS[p4opts]}")"

    IFS=" "
    $PYTHON -B src/compiler.py ${OPTS[p4opts]}
    exit_on_error "P4 to C compilation failed"
fi


# Phase 2: C compilation
if [ "$(optvalue c)" != off ]; then
    cat <<EOT > "/tmp/${T4P4S_GEN_INCLUDE}.tmp"
#ifndef __GEN_INCLUDE_H_
#define __GEN_INCLUDE_H_
EOT

    for colour in ${ALL_COLOUR_NAMES[@]}; do
        COLOUR_MACRO=""
        [ "$(array_contains "${OPTS[bw]}" "on" "switch")" == n ] && COLOUR_MACRO="\"${OPTS[${OPTS[T4LIGHT_$colour]}]-${OPTS[T4LIGHT_$colour]}}\"  // ${OPTS[T4LIGHT_$colour]}"
        [ "$(array_contains "${OPTS[bw]}" "on" "switch")" == n ] && [ "$COLOUR_MACRO" == "\"\"" ] && [ "$colour" != "default" ] && COLOUR_MACRO="T4LIGHT_default"
        echo "#define T4LIGHT_${colour} $COLOUR_MACRO" >> "/tmp/${T4P4S_GEN_INCLUDE}.tmp"
    done

    IFS=" "
    for hdr in ${OPTS[include-hdrs]}; do
        echo "#include \"$hdr\"" >> "/tmp/${T4P4S_GEN_INCLUDE}.tmp"
    done

    echo "#endif" >> "/tmp/${T4P4S_GEN_INCLUDE}.tmp"
    overwrite_on_difference "${T4P4S_GEN_INCLUDE}" "${T4P4S_GEN_INCLUDE_DIR}"


    cat <<EOT >"/tmp/${GEN_MAKEFILE}.tmp"
CDIR := \$(dir \$(lastword \$(MAKEFILE_LIST)))
APP = ${OPTS[example]}
include \$(CDIR)/../../makefiles/${ARCH}_backend_pre.mk
include \$(CDIR)/../../makefiles/common.mk
include \$(CDIR)/../../makefiles/hw_independent.mk
VPATH += $(dirname ${OPTS[source]})
EXTRA_CFLAGS += ${OPTS[extra-cflags]}
EOT

    if [ "$(optvalue testcase)" != off -o "$(optvalue suite)" != off ]; then
        TESTDIR="examples"
        if [ $(find "$TESTDIR" -type f -name "test-${OPTS[example]##test-}.c" | wc -l) -ne 1 ]; then
            errmsg_exit "Test data file $(cc 0)$TESTFILE$nn in $(cc 0)$TESTDIR$nn not found"
        fi
        TESTFILE=$(find "$TESTDIR" -type f -name "test-${OPTS[example]##test-}.c")

        [ "$(optvalue testcase)" != off -a "$(optvalue suite)" == off ] && addopt cflags "-DT4P4S_TESTCASE=\"t4p4s_testcase_${OPTS[testcase]}\"" " "
        echo "VPATH += \$(CDIR)/../../`dirname $TESTFILE`" >>"/tmp/${GEN_MAKEFILE}.tmp"
        echo "SRCS-y += `basename $TESTFILE`" >>"/tmp/${GEN_MAKEFILE}.tmp"
    fi

    IFS=" "
    for src in ${OPTS[include-srcs]}; do
        echo "SRCS-y += $src" >> "/tmp/${GEN_MAKEFILE}.tmp"
    done

    echo "CFLAGS += ${OPTS[cflags]}" >> "/tmp/${GEN_MAKEFILE}.tmp"
    echo "include \$(CDIR)/../../makefiles/${ARCH}_backend_post.mk" >> "/tmp/${GEN_MAKEFILE}.tmp"

    overwrite_on_difference "${GEN_MAKEFILE}" "${GEN_MAKEFILE_DIR}"


    msg "[$(cc 0)COMPILE SWITCH$nn]"
    verbosemsg "C compiler options: $(cc 0)$(print_cmd_opts "${OPTS[cflags]}")${nn}"

    cd ${T4P4S_TARGET_DIR}
    if [ "$(optvalue silent)" != off ]; then
        make -j >/dev/null
    else
        make -j
    fi
    exit_on_error "C compilation failed"

    cd - >/dev/null
fi


if [ "$(optvalue run)" != off ]; then
    if [ "$(optvalue ctr)" == off ]; then
        msg "[$(cc 0)NO  CONTROLLER$nn]"
    else
        mkdir -p ${T4P4S_LOG_DIR}

        CONTROLLER="dpdk_${OPTS[ctr]}_controller"
        CONTROLLER_LOG=${T4P4S_LOG_DIR}/controller.log

        sudo killall -q "$CONTROLLER"

        msg "[$(cc 0)RUN CONTROLLER$nn] $(cc 1)${CONTROLLER}$nn (default for $(cc 0)${OPTS[example]}$nn@$(cc 1)${OPTS[variant]}$nn)"

        verbosemsg "Controller log : $(cc 0)${CONTROLLER_LOG}$nn"
        verbosemsg "Controller opts: $(print_cmd_opts ${OPTS[ctrcfg]})"

        # Step 3A-1: Compile the controller
        cd $CTRL_PLANE_DIR
        if [ "$(optvalue silent)" != off ]; then
            make -s -j $CONTROLLER >/dev/null
        else
            make -s -j $CONTROLLER
        fi
        exit_on_error "Controller compilation failed"
        cd - >/dev/null

        # Step 3A-3: Run controller
        if [ $(optvalue showctl optv) == y ]; then
            stdbuf -o 0 $CTRL_PLANE_DIR/$CONTROLLER ${OPTS[ctrcfg]} &
        else
            (stdbuf -o 0 $CTRL_PLANE_DIR/$CONTROLLER ${OPTS[ctrcfg]} >&2> "${CONTROLLER_LOG}" &)
        fi
        sleep 0.05
    fi
fi


# Phase 3B: Execution (switch)
if [ "$(optvalue run)" != off ]; then
    msg "[$(cc 0)RUN SWITCH$nn] $(cc 1)${OPTS[executable]}$nn"

    sudo mkdir -p /mnt/huge

    grep -s '/mnt/huge' /proc/mounts > /dev/null
    if [ $? -ne 0 ] ; then
        sudo mount -t hugetlbfs nodev /mnt/huge
    fi

    [ "$(optvalue hugepages)" != off ] && reserve_hugepages2 "${OPTS[hugepages]}"

    [ "$ARCH" == "dpdk" ] && EXEC_OPTS="${OPTS[ealopts]} -- ${OPTS[cmdopts]}"

    verbosemsg "Options    : $(print_cmd_opts "${EXEC_OPTS}")"

    mkdir -p ${T4P4S_LOG_DIR}
    echo "Executed at $(date +"%Y%m%d %H:%M:%S")" >${T4P4S_LOG_DIR}/last.txt
    echo >>${T4P4S_LOG_DIR}/last.txt
    if [ "${OPTS[eal]}" == "off" ]; then
        sudo -E "${OPTS[executable]}" ${EXEC_OPTS} 2>&1 | egrep -v "^EAL: " \
            |& tee >( tee -a ${T4P4S_LOG_DIR}/last.lit.txt | sed 's/\x1B\[[0-9;]*[JKmsu]//g' >> ${T4P4S_LOG_DIR}/last.txt ) \
            |& tee >( tee ${T4P4S_LOG_DIR}/$(date +"%Y%m%d_%H%M%S")_${OPTS[choice]}.lit.txt | sed 's/\x1B\[[0-9;]*[JKmsu]//g' > ${T4P4S_LOG_DIR}/$(date +"%Y%m%d_%H%M%S")_${OPTS[choice]}.txt )
        # note: PIPESTATUS is bash specific
        ERROR_CODE=${PIPESTATUS[0]}
    else
        sudo -E "${OPTS[executable]}" ${EXEC_OPTS} \
            |& tee >( tee -a ${T4P4S_LOG_DIR}/last.lit.txt | sed 's/\x1B\[[0-9;]*[JKmsu]//g' >> ${T4P4S_LOG_DIR}/last.txt ) \
            |& tee >( tee ${T4P4S_LOG_DIR}/$(date +"%Y%m%d_%H%M%S")_${OPTS[choice]}.lit.txt | sed 's/\x1B\[[0-9;]*[JKmsu]//g' > ${T4P4S_LOG_DIR}/$(date +"%Y%m%d_%H%M%S")_${OPTS[choice]}.txt )
        ERROR_CODE=${PIPESTATUS[0]}
    fi

    command -v errno >&2>/dev/null
    ERRNO_EXISTS=$?
    [ $ERRNO_EXISTS -eq 0 ] && [ $ERROR_CODE -eq 0 ] && ERR_CODE_MSG="($(cc 0)`errno $ERROR_CODE`$nn)"
    [ $ERRNO_EXISTS -eq 0 ] && [ $ERROR_CODE -ne 0 ] && ERR_CODE_MSG="($(cc 3 2 1)`errno $ERROR_CODE`$nn)"

    [ $ERROR_CODE -eq 139 ] && ERR_CODE_MSG="($(cc 3 2 1)Segmentation fault$nn)"
    [ $ERROR_CODE -eq 255 ] && ERR_CODE_MSG="($(cc 2 1)Switch execution error$nn)"

    [ $ERROR_CODE -eq 0 ] && msg "${nn}T4P4S switch exited $(cc 0)normally$nn"
    [ $ERROR_CODE -ne 0 ] && msg "\n${nn}T4P4S switch exited with error code $(cc 3 2 1)$ERROR_CODE$nn $ERR_CODE_MSG"
    [ $ERROR_CODE -ne 0 ] && msg " - Runtime options were: $(print_cmd_opts "${EXEC_OPTS}")"

    DBGWAIT=3
    if [ $ERROR_CODE -ne 0 ] && [ "$(optvalue autodbg)" != off ]; then
        msg "Running $(cc 1)debugger $DEBUGGER$nn in $(cc 0)$DBGWAIT$nn seconds"
        sleep $DBGWAIT
        sudo -E ${DEBUGGER} -q -ex run --args "${OPTS[executable]}" ${EXEC_OPTS}
    fi
fi

exit_program $ERROR_CODE
