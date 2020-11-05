#!/bin/bash

declare -A KNOWN_COLOURS
declare -A OPTS
declare -A IGNORE_OPTS

PYTHON_PARSE_HELPER_PROCESS=""

# --------------------------------------------------------------------
# Show customisable environment values in this file

if [ $# == 1 ] && [ "$1" == "showenvs" ]; then
    escape_char=$(printf '\033')
    colours=([0]="${escape_char}[1;32m" [1]="${escape_char}[1;33m" [2]="${escape_char}[1;31m")
    nn="${escape_char}[0m"

    echo "The ${colours[0]}$0$nn script uses the following ${colours[1]}default values$nn for ${colours[0]}environment variables$nn."
    cat t4p4s.sh | grep -e '\([A-Z_]*\)=[$][{]\1-' | sed "s/[ ]*\([^=]*\)=[$][{]\1-\(.*\)[}]$/    ${colours[0]}\1$nn=${colours[1]}\2$nn/"
    echo "Override them like this to customise the script's behaviour."
    echo "    ${colours[0]}EXAMPLES_CONFIG_FILE$nn=${colours[1]}my_config.cfg$nn ${colours[0]}$0$nn ${colours[1]}:my_p4$nn"
    echo "    ${colours[0]}EXAMPLES_CONFIG_FILE$nn=${colours[1]}my_config.cfg$nn ${colours[0]}COLOURS_CONFIG_FILE$nn=${colours[1]}my_favourite_colours.cfg$nn ${colours[0]}$0$nn ${colours[1]}%my_p4 dbg verbose p4testcase=1$nn"
    exit
fi

# --------------------------------------------------------------------
# Helper functions

exit_program() {
    echo -e "$nn"
    [ "${OPTS[ctr]}" != "" ] && verbosemsg "(Terminating controller $(cc 0)dpdk_${OPTS[ctr]}_controller$nn)" && sudo killall -q "dpdk_${OPTS[ctr]}_controller"
    [ "${PYTHON_PARSE_HELPER_PROCESS}" != "" ] && verbosemsg "(Terminating the $(cc 0)parse helper process$nn, port $(cc 1)$PYTHON_PARSE_HELPER_PORT$nn, pid $(cc 1)$PYTHON_PARSE_HELPER_PROCESS$nn)" && (echo "exit_parse_helper" | netcat localhost "${PYTHON_PARSE_HELPER_PORT}")
    [ "$1" != "" ] && errmsg "$(cc 3)Error$nn: $*"
    exit $ERROR_CODE
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

exit_on_error() {
    ERROR_CODE=$1
    [ "$ERROR_CODE" -eq 0 ] && return

    exit_program "$2 (error code: $(cc 2)$ERROR_CODE$nn)"
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

get_current_envs() {
    ( set -o posix ; set ) | grep -v "PYTHON_PARSE_HELPER_PROCESS" | tr '\n' '\r' | sed "s/\r[^=]*='[^']*\r[^\r]*'\r/\r/g" | tr '\r' '\n'
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

ctrl_c_handler() {
    (>&2 echo -e "\nInterrupted, exiting...")
    ERROR_CODE=254
    exit_program
}

trap 'ctrl_c_handler' INT


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

# --------------------------------------------------------------------

# from $RTE_SDK/usertools/dpdk-setup.py
remove_mnt_huge()
{
    grep -s '/mnt/huge' /proc/mounts > /dev/null
    if [ $? -eq 0 ] ; then
        sudo umount /mnt/huge
    fi

    if [ -d /mnt/huge ] ; then
        sudo rm -R /mnt/huge
    fi
}

# from $RTE_SDK/usertools/dpdk-setup.py
clear_huge_pages()
{
    echo > .echo_tmp
    for d in /sys/devices/system/node/node? ; do
        echo "echo 0 > $d/hugepages/hugepages-${HUGEPGSZ}/nr_hugepages" >> .echo_tmp
    done
    sudo sh .echo_tmp
    rm -f .echo_tmp

    remove_mnt_huge
}

# from $RTE_SDK/usertools/dpdk-setup.py
create_mnt_huge()
{
    sudo mkdir -p /mnt/huge

    grep -s '/mnt/huge' /proc/mounts > /dev/null
    if [ $? -ne 0 ] ; then
        sudo mount -t hugetlbfs nodev /mnt/huge
    fi
}

change_hugepages() {
    [ "$(optvalue hugepages)" == keep ] && return

    HUGEPGSZ=`cat /proc/meminfo | grep Hugepagesize | cut -d : -f 2 | tr -d ' '`
    HUGE_FORMULA=`echo $HUGEPGSZ | sed -e 's/kB//' | sed -e 's/MB/*1024/' | sed -e 's/GB/*1024*1024/'`

    OLD_HUGEPAGES=`cat /sys/kernel/mm/hugepages/hugepages-${HUGEPGSZ}/nr_hugepages`

    [ "$(optvalue hugepages)" == "off" ] && [ "$(optvalue hugemb)" != "off" ] && OPTS[hugepages]=$(($(optvalue hugemb)*1024/($HUGE_FORMULA) + (($(optvalue hugemb)*1024%($HUGE_FORMULA) > 0))))

    HUGE_UNIT=kB
    HUGE_KB=$(($HUGE_FORMULA*OPTS[hugepages]))
    HUGE_AMOUNT=$HUGE_KB
    [ "$(($HUGE_KB % 1024))" -eq 0 ] && HUGE_UNIT=MB && HUGE_AMOUNT=$(($HUGE_KB / 1024))
    [ "$(($HUGE_KB % 1048576))" -eq 0 ] && HUGE_UNIT=GB && HUGE_AMOUNT=$(($HUGE_KB / 1048576))

    if [ $OLD_HUGEPAGES -eq 0 ]; then
        verbosemsg "Allocating $(cc 0)${OPTS[hugepages]}$nn hugepages ($(cc 0)$HUGE_AMOUNT$nn $HUGE_UNIT)"
    elif [ $OLD_HUGEPAGES -lt ${OPTS[hugepages]} ]; then
        verbosemsg "Increasing hugepage count from $(cc 1)$OLD_HUGEPAGES$nn to $(cc 0)${OPTS[hugepages]}$nn ($(cc 0)$HUGE_AMOUNT$nn $HUGE_UNIT)"
    elif [ "$(optvalue hugeopt)" == exact ] && [ $OLD_HUGEPAGES -gt ${OPTS[hugepages]} ]; then
        verbosemsg "Reducing hugepage count from $(cc 1)$OLD_HUGEPAGES$nn to $(cc 0)${OPTS[hugepages]}$nn ($(cc 0)$HUGE_AMOUNT$nn $HUGE_UNIT)"
    elif [ $OLD_HUGEPAGES -eq ${OPTS[hugepages]} ]; then
        verbosemsg "Keeping the previously reserved $(cc 0)$OLD_HUGEPAGES$nn hugepages ($(cc 0)$HUGE_AMOUNT$nn $HUGE_UNIT)"
        return
    else
        verbosemsg "Keeping $(cc 0)$OLD_HUGEPAGES$nn hugepages which is more than the requested $(cc 0)${OPTS[hugepages]}$nn ($(cc 0)$HUGE_AMOUNT$nn $HUGE_UNIT)"
        return
    fi

    clear_huge_pages
    echo "echo ${OPTS[hugepages]} > /sys/kernel/mm/hugepages/hugepages-${HUGEPGSZ}/nr_hugepages" > .echo_tmp
    sudo sh .echo_tmp
    rm -f .echo_tmp
    create_mnt_huge

    NEW_HUGEPAGES=`cat /sys/kernel/mm/hugepages/hugepages-${HUGEPGSZ}/nr_hugepages`
    [ $NEW_HUGEPAGES -lt ${OPTS[hugepages]} ] && exit_on_error "1" "Was asked to reserve $(cc 0)${OPTS[hugepages]}$nn hugepages, got $(cc 3)only ${NEW_HUGEPAGES}$nn"
}

# --------------------------------------------------------------------

# /tmp/$1.tmp is a file that has some (generated) contents; $2/$1 may or may not exist
# Only (over)write $1 if the generated content differs from the existing one
overwrite_on_difference() {
    cmp -s "/tmp/$1.tmp" "$2/$1"
    [ "$?" -ne 0 ] && sudo mv "/tmp/$1.tmp" "$2/$1"
    sudo rm -f "/tmp/$1.tmp"
}

candidate_count() {
    simple_count=$(find -L "$P4_SRC_DIR" -type f -name "$1.p4*" | wc -l)
    if [ $simple_count -eq 1 ]; then
        echo 1
    else
        echo $(find -L "$P4_SRC_DIR" -type f -regex ".*$1.*[.]p4\(_[0-9][0-9]*\)?" | wc -l)
    fi
}

candidates() {
    if [ $(candidate_count $1) -gt 0 ]; then
        echo
        find -L "$P4_SRC_DIR" -type f -regex ".*$1.*[.]p4\(_[0-9][0-9]*\)?" | sed 's#^.*/\([^\.]*\).*$#    \1#g'
    else
        echo "(no candidates found)"
    fi
}

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
CFGFILES=${CFGFILES-${COLOURS_CONFIG_FILE},${LIGHTS_CONFIG_FILE},!cmdline!,!varcfg!${EXAMPLES_CONFIG_FILE},${ARCH_OPTS_FILE}}

POSSIBLE_PYTHONS=(python3.10 python3.9 python3.8 python3)
for found_python3 in "${POSSIBLE_PYTHONS[@]}"
do
    if [ `command -v "${found_python3}"` ]; then
        PYTHON=${PYTHON-${found_python3}}
        break
    fi
done
DEBUGGER=${DEBUGGER-gdb}

declare -A EXT_TO_VSN=(["p4"]=16 ["p4_14"]=14)

# --------------------------------------------------------------------

# Generating random ephemeral port
while
    PYTHON_PARSE_HELPER_PORT=$(shuf -n 1 -i 49152-65535)
    netstat -atun | grep -q "$PYTHON_PARSE_HELPER_PORT"
do
    continue
done

PYTHON_PARSE_HELPER=$(cat <<END
#!/usr/bin/env ${PYTHON}

import socket
import sys
import re

HOST = 'localhost'
PORT = int(sys.argv[1])

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

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    s.listen()

    while True:
        conn, addr = s.accept()
        with conn:
            bline = conn.recv(1024)
            line = bline.decode()
            if line.startswith("exit_parse_helper"):
                sys.exit()

            m = re.match(rexp, line)

            is_ok = 'y' if m else 'n'
            conn.sendall(f'ok {is_ok}\n'.encode())
            for gname in (p[0] for p in patterns):
                opt = '' if not m else m.group(gname) or ''
                conn.sendall(f'{gname} {opt}\n'.encode())
END
    )

$PYTHON -c "$PYTHON_PARSE_HELPER" "${PYTHON_PARSE_HELPER_PORT}" >&2>/dev/null &
PYTHON_PARSE_HELPER_PROCESS="$!"

unset PYTHON_PARSE_HELPER

sleep 0.1


# --------------------------------------------------------------------
# Set defaults

ALL_COLOUR_NAMES=(action bytes control core default error expected extern field header headertype incoming off outgoing packet parserstate port smem socket status success table testcase warning)

# --------------------------------------------------------------------
# Set defaults

colours=()
nn="\033[0m"

# Check if configuration is valid
[ "${P4C}" == "" ] && exit_program "\$P4C not defined"
[ "$ARCH" == "dpdk" ] && [ "${RTE_SDK}" == "" ] && exit_program "\$RTE_SDK not defined"

# --------------------------------------------------------------------
# Parse opts from files and command line

OPT_NOPRINTS=("OPT_NOPRINTS" "cfgfiles")

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
                    if [[ $opt == @* ]]; then
                        collected_opts=""
                        # option can refers to another option in the same file
                        while read -r opts2; do
                            IFS=' ' read -r -a optparts2 <<< `echo $opts2 | sed -e "s/^$opt//g"`

                            # skip the first element, which is textually the same as $opt
                            for opt2 in ${optparts2[@]}; do
                                NEWOPTS+=("$opt2")
                            done
                        done < <(cat "${cfgfile#!varcfg!}" | grep -e "^$opt\s" | sed -e "s/^[^ \t]+[ \t]*//g")
                    else
                        NEWOPTS+=("$opt")
                    fi
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
            if [[ "$opt" == *.p4* ]] && [ -f "$opt" ]; then
                setopt example "$(basename ${opt%.*})"
                setopt source "$opt"
                continue
            fi


            # Split the option into its components along the above regex
            unset groups
            declare -A groups=()
            IFS=' '
            while read -r grpid grptxt; do groups["$grpid"]="$grptxt"; done < <(echo "$opt" | netcat localhost "${PYTHON_PARSE_HELPER_PORT}" | grep -ve "^$")

            [ "${groups[ok]}" == n ] && [[ $opt = *\;* ]] && continue
            [ "${groups[ok]}" == n ] && echo -e "Cannot parse option $(cc 0)$opt$nn (origin: $OPT_ORIGIN)" && continue

            var="${groups[var]}"
            [ "${groups[neg]}" != "" ] && OPTS[$var]=off && continue

            if [ "${groups[cond]}" != "" ]; then
                expected_value="${groups[condval]}"
                [ "$(optvalue "${groups[condvar]}")" == off -a "$expected_value" != "off" ] && continue
                [ "$expected_value" != "" -a "${OPTS[${groups[condvar]}]}" != "$expected_value" ] && continue

                letval_cond=${groups[condvar]}
                letval_value=$(optvalue ${groups[condvar]})
                groups[letval]=`echo ${groups[letval]} | sed -e "s/[$][\{]${letval_cond}[\}]/${letval_value}/g"`
            fi

            value="${groups[letval]:-on}"

            [[ $var == COLOUR_* ]] && KNOWN_COLOURS[${var#COLOUR_}]="$value"
            [ "$var" == "light" ] && set_term_light "$value" && continue

            [ "$var" == cfgfiles -a ! -f "$value" ] && echo -e "Config file $(cc 0)$value$nn cannot be found" && continue

            if [ "$(array_contains "${groups[prefix]}" ":" "::" "%" "%%")" == y ]; then
                FIND_COUNT=$(candidate_count "${var}")
                [ $FIND_COUNT -gt 1 ] && exit_program "Name is not unique: found $(cc 1)$FIND_COUNT$nn P4 files for $(cc 0)${var}$nn, candidates: $(cc 1)$(candidates ${var})$nn"
                [ $FIND_COUNT -eq 0 ] && exit_program "Could not find P4 file for $(cc 0)${var}$nn, candidates: $(cc 1)$(candidates ${var})$nn"

                setopt example "$var"
                P4_SRC_FILE="`find -L "$P4_SRC_DIR" -type f -regex ".*/${var}[.]p4\(_[0-9][0-9]*\)?"`"
                setopt source "$P4_SRC_FILE"
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
            [ "$(optvalue example)" == off ] && exit_program "No example to run"
            # The variant has to be determined before processing the config files.
            [ "$(optvalue variant)" == off ] && setopt variant std
        fi
    done
done

verbosemsg "Python 3   is $(cc 0)$PYTHON$nn"
verbosemsg "Debugger   is $(cc 0)$DEBUGGER$nn"
verbosemsg "Parse port is $(cc 0)$PYTHON_PARSE_HELPER_PORT$nn"

[ "$(optvalue verbose)" == on ] && IGNORE_OPTS[silent]=on
[ "$(optvalue silent)" == on  ] && IGNORE_OPTS[verbose]=on


[ "$(optvalue variant)" == off ] && [ "$(optvalue testcase)" != off -o "$(optvalue suite)" != off ] && OPTS[variant]=test && verbosemsg "Variant $(cc 1)@test$nn is chosen because testing is requested"
[ "${OPTS[variant]}" == "" -o "${OPTS[variant]}" == "-" ] && OPTS[variant]=std && verbosemsg "Variant $(cc 1)@std$nn is automatically chosen"


# Determine version by extension if possible
if [ "${OPTS[vsn]}" == "" ]; then
    P4_EXT="$(basename "${OPTS[source]}")"
    P4_EXT=${P4_EXT##*.}
    if [ "$(array_contains "${P4_EXT##*.}" ${!EXT_TO_VSN[@]})" == n ]; then
        exit_program "Cannot determine P4 version for the extension $(cc 0)${P4_EXT}$nn of $(cc 0)$(print_cmd_opts "${OPTS[source]}")$nn"
    fi
    OPTS[vsn]="${EXT_TO_VSN["${P4_EXT##*.}"]}"
    [ "${OPTS[vsn]}" == "" ] && exit_program "Cannot determine P4 version for $(cc 0)${OPTS[example]}$nn"
    verbosemsg "P4 version is $(cc 0)${OPTS[vsn]}$nn (by the extension of $(cc 0)$(print_cmd_opts "${OPTS[source]}")$nn)"
fi

[ "$(optvalue testcase)" == off ] && OPTS[choice]=${T4P4S_CHOICE-${OPTS[example]}@${OPTS[variant]}}
[ "$(optvalue testcase)" != off ] && OPTS[choice]=${T4P4S_CHOICE-${OPTS[example]}@${OPTS[variant]}-$(optvalue testcase)}
T4P4S_BUILD_DIR=${T4P4S_BUILD_DIR-"./build"}
T4P4S_COMPILE_DIR=${T4P4S_COMPILE_DIR-"${T4P4S_BUILD_DIR}/${OPTS[choice]}"}
T4P4S_TARGET_DIR="${T4P4S_BUILD_DIR}/last"

OPTS[executable]="$T4P4S_TARGET_DIR/build/${OPTS[example]}"

T4P4S_SRCGEN_DIR=${T4P4S_SRCGEN_DIR-"$T4P4S_TARGET_DIR/srcgen"}
T4P4S_GEN_INCLUDE_DIR="${T4P4S_SRCGEN_DIR}"
T4P4S_GEN_INCLUDE="gen_include.h"

T4P4S_LOG_DIR=${T4P4S_LOG_DIR-$(dirname $(dirname ${OPTS[executable]}))/log}

EXAMPLES_DIR=${EXAMPLES_DIR-./examples}

# By default use all three phases
if [ "$(optvalue p4)" == off ] && [ "$(optvalue c)" == off ] && [ "$(optvalue run)" == off ]; then
    OPTS[p4]=on
    OPTS[c]=on
    OPTS[run]=on
fi

T4P4S_CC=${T4P4S_CC-gcc}
which clang >/dev/null
[ $? -eq 0 ] && T4P4S_CC=clang

T4P4S_LD=${T4P4S_LD-bfd}
which lld >/dev/null
[ $? -eq 0 ] && T4P4S_LD=lld

MESON=${MESON-meson}
NINJA=${NINJA-ninja}

[ "$(optvalue silent)" != off ] && addopt makeopts ">/dev/null" " "

# Generate directories and files
mkdir -p "$T4P4S_COMPILE_DIR"
rm -f "$T4P4S_TARGET_DIR"
ln -s "`realpath "$T4P4S_COMPILE_DIR"`" "$T4P4S_TARGET_DIR"
mkdir -p $T4P4S_SRCGEN_DIR

# --------------------------------------------------------------------
# Checks before execution of phases begins

if [ "$(optvalue testcase)" != off -o "$(optvalue suite)" != off ]; then
    if [ $(find -L "$EXAMPLES_DIR" -type f -name "test-${OPTS[example]##test-}.c" | wc -l) -ne 1 ]; then
        exit_program "No test input file found for example $(cc 0)${OPTS[example]}$nn under $(cc 0)$EXAMPLES_DIR$nn (expected filename: $(cc 0)test-${OPTS[example]##test-}.c$nn)"
    fi
fi

# --------------------------------------------------------------------
# Environment variable printout

if [ "${OPTS[showenvs]}" != "" ]; then
    echo -e "The following values have been computed for the $(cc 0)customisable environment variables$nn."
    IFS=$'\n'
    for envvar in `cat t4p4s.sh | grep -e '\([A-Z_]*\)=[$][{]\1-' | sed "s/[ ]*\([^=]*\)=[$][{]\1-\(.*\)[}]$/\1/"`; do
        if [ -z "${!envvar}" ]; then
            echo -e "    $(cc 0)$envvar$nn $(cc 2)is not set$nn"
        else
            echo -e "    $(cc 0)$envvar$nn=$(cc 1)${!envvar}$nn"
        fi
    done
fi

# --------------------------------------------------------------------

verbosemsg "Options: $(print_opts)"

# Phase 0a: Check for required programs
if [ "$(optvalue c)" != off -a ! -f "$P4C/build/p4test" ]; then
    exit_program "cannot find P4C compiler at $(cc 1)\$P4C/build/p4test$nn"
fi


# Phase 0b: If a phase with root access is needed, ask for it now
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
    exit_on_error "$?" "P4 to C compilation $(cc 2)failed$nn"
fi


# Phase 2: C compilation
if [ "$(optvalue c)" != off ]; then
    sudo echo "#pragma once" > "/tmp/${T4P4S_GEN_INCLUDE}.tmp"

    unset colour
    for colour in ${ALL_COLOUR_NAMES[@]}; do
        COLOUR_MACRO=""
        [ "$(array_contains "${OPTS[bw]}" "on" "switch")" == n ] && COLOUR_MACRO="\"${OPTS["${OPTS[T4LIGHT_$colour]}"]-${OPTS[T4LIGHT_$colour]}}\"  // ${OPTS[T4LIGHT_$colour]}"
        [ "$(array_contains "${OPTS[bw]}" "on" "switch")" == n ] && [ "$COLOUR_MACRO" == "\"\"" ] && [ "$colour" != "default" ] && COLOUR_MACRO="T4LIGHT_default"
        sudo echo "#define T4LIGHT_${colour} $COLOUR_MACRO" >> "/tmp/${T4P4S_GEN_INCLUDE}.tmp"
    done

    IFS=" "
    for hdr in ${OPTS[include-hdrs]}; do
        sudo echo "#include \"$hdr\"" >> "/tmp/${T4P4S_GEN_INCLUDE}.tmp"
    done

    overwrite_on_difference "${T4P4S_GEN_INCLUDE}" "${T4P4S_GEN_INCLUDE_DIR}"


    sudo cat <<EOT >"/tmp/meson.build.tmp"
project(
    '${OPTS[example]}',
    'c',
    version : '1.0.0',
    default_options : [
        'warning_level=0'
    ],
)
EOT

    sudo cat "meson.build.base" >>"/tmp/meson.build.tmp"

    if [ "$(optvalue testcase)" != off -o "$(optvalue suite)" != off ]; then
        TESTFILE=$(find -L "$EXAMPLES_DIR" -type f -name "test-${OPTS[example]##test-}.c")

        [ "$(optvalue testcase)" != off -a "$(optvalue suite)" == off ] && addopt cflags "-DT4P4S_TESTCASE=t4p4s_testcase_${OPTS[testcase]}" " "

        sudo echo >>"/tmp/meson.build.tmp"
        sudo echo "project_source_files += ['../../$TESTFILE']" >>"/tmp/meson.build.tmp"
        sudo echo >>"/tmp/meson.build.tmp"
    fi

    IFS=" "
    for src in ${OPTS[include-srcs]}; do
        sudo echo "project_source_files += ['$src']" >> "/tmp/meson.build.tmp"
    done

    for flag in ${OPTS[cflags]}; do
        sudo echo "build_args += ['$flag']" >> "/tmp/meson.build.tmp"
    done


    sudo cat <<EOT >>"/tmp/meson.build.tmp"
executable(
    meson.project_name(),
    project_source_files,
    c_args                : build_args,
    gnu_symbol_visibility : 'hidden',
    include_directories   : include_dirs,
    dependencies : [
        dependency('libdpdk'),
        dependency('threads'),
    ],
)
EOT

    overwrite_on_difference "meson.build" "${T4P4S_TARGET_DIR}"


    msg "[$(cc 0)COMPILE SWITCH$nn]"
    verbosemsg "C compiler options: $(cc 0)$(print_cmd_opts "${OPTS[cflags]}")${nn}"

    if [ ! -d ${T4P4S_TARGET_DIR}/build ];  then
        cd ${T4P4S_TARGET_DIR}
        CC="ccache $T4P4S_CC" CC_LD="$T4P4S_LD" $MESON build >/dev/null 2>/dev/null
        exit_on_error "$?" "Meson invocation $(cc 2)failed$nn"

        cd - >/dev/null
    fi

    cd ${T4P4S_TARGET_DIR}/build
    sudo $NINJA  >/dev/null 2>/dev/null
    [ "$(optvalue c)" == "rebuild" ] && sudo $NINJA clean
    sudo $NINJA
    exit_on_error "$?" "C compilation using ninja $(cc 2)failed$nn"
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

        for ctrcfg in ${OPTS[ctrcfg]}; do
            [ ! -f $ctrcfg ] && exit_program "Controller config file $(cc 2)$ctrcfg$nn does not exist"
        done

        verbosemsg "Controller log : $(cc 0)${CONTROLLER_LOG}$nn"
        verbosemsg "Controller opts: $(print_cmd_opts ${OPTS[ctrcfg]})"

        # Step 3A-1: Compile the controller
        cd $CTRL_PLANE_DIR
        if [ "$(optvalue verbose)" == on ]; then
            CC="ccache $T4P4S_CC" LD="ld.$T4P4S_LD" make -j $CONTROLLER
        elif [ "$(optvalue silent)" != off ]; then
            CC="ccache $T4P4S_CC" LD="ld.$T4P4S_LD" make -s -j $CONTROLLER
        else
            CC="ccache $T4P4S_CC" LD="ld.$T4P4S_LD" make -s -j $CONTROLLER >/dev/null
        fi
        exit_on_error "$?" "Controller compilation $(cc 2)failed$nn"
        cd - >/dev/null

        command -v gnome-terminal >/dev/null 2>/dev/null
        HAS_TERMINAL=$?

        # Step 3A-3: Run controller
        if [ $(optvalue showctl optv) == y ]; then
            stdbuf -o 0 $CTRL_PLANE_DIR/$CONTROLLER ${OPTS[ctrcfg]} &
        elif [ "$(optvalue ctrterm)" != off -a "$HAS_TERMINAL" == "0" ]; then
            TERMWIDTH=${TERMWIDTH-72}
            TERMHEIGHT=${TERMHEIGHT-36}
            gnome-terminal --geometry ${TERMWIDTH}x${TERMHEIGHT} -- bash -c "echo Example: ${OPTS[source]} @${OPTS[variant]} && echo Controller: ${CONTROLLER} && echo && (stdbuf -o 0 $CTRL_PLANE_DIR/$CONTROLLER ${OPTS[ctrcfg]} | tee ${CONTROLLER_LOG}); read -p 'Press Return to close window'" 2>/dev/null
        else
            (stdbuf -o 0 $CTRL_PLANE_DIR/$CONTROLLER ${OPTS[ctrcfg]} >&2> "${CONTROLLER_LOG}" &)
        fi
        sleep 0.2
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

    change_hugepages


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
    [ $ERROR_CODE -ne 0 ] && msg "\n${nn}T4P4S switch running $(cc 0)$(print_cmd_opts "${OPTS[source]}")$nn exited with error code $(cc 3 2 1)$ERROR_CODE$nn $ERR_CODE_MSG"
    [ $ERROR_CODE -ne 0 ] && msg " - Runtime options were: $(print_cmd_opts "${EXEC_OPTS}")"

    DBGWAIT=1
    if [ $ERROR_CODE -ne 0 ] && [ "$(optvalue autodbg)" != off ]; then
        [ "${OPTS[ctr]}" != "" ] && verbosemsg "Restarting controller $(cc 0)dpdk_${OPTS[ctr]}_controller$nn" && sudo killall -q "dpdk_${OPTS[ctr]}_controller"
        (stdbuf -o 0 $CTRL_PLANE_DIR/$CONTROLLER ${OPTS[ctrcfg]} &)

        msg "Running $(cc 1)debugger $DEBUGGER$nn in $(cc 0)$DBGWAIT$nn seconds"
        sleep $DBGWAIT
        print "${OPTS[executable]}"
        sudo -E ${DEBUGGER} -q -ex run --args "${OPTS[executable]}" ${EXEC_OPTS}
    fi
fi

exit_program
