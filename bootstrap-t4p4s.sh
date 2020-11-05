
# Highlight colours
cc="\033[1;33m"     # yellow
nn="\033[0m"

if [ $# -gt 0 ] && [ "$1" == "showenvs" ]; then
    escape_char=$(printf '\033')
    colours=([0]="${escape_char}[1;32m" [1]="${escape_char}[1;33m" [2]="${escape_char}[1;31m")
    nn="${escape_char}[0m"

    echo "The ${colours[0]}$0$nn script uses the following ${colours[1]}default values$nn for ${colours[0]}environment variables$nn."
    cat "$0" | grep -e '\([A-Z0-9_]*\)=[$][{]\1-'| sed "s/[ ]*export//" | sed "s/[ ]*\([^=]*\)=[$][{]\1-\(.*\)[}]$/    ${colours[0]}\1$nn=${colours[1]}\2$nn/" | sort
    echo "Override them like this to customise the script's behaviour."
    echo "    ${colours[0]}MAX_MAKE_JOBS$nn=${colours[1]}8$nn ${colours[0]}T4P4S_CC$nn=${colours[1]}clang$nn ${colours[0]}$0$nn"
    exit
fi

WORKDIR=`pwd`

SYSTEM_THREAD_COUNT=`nproc --all`

MAX_MAKE_JOBS=${MAX_MAKE_JOBS-$SYSTEM_THREAD_COUNT}
FRESH=${FRESH-yes}
CLEANUP=${CLEANUP-yes}
USE_OPTIONAL_PACKAGES=${USE_OPTIONAL_PACKAGES-yes}
PARALLEL_INSTALL=${PARALLEL_INSTALL-yes}

MESON=${MESON-meson}
NINJA=${NINJA-ninja}

INSTALL_STAGE1_APTGET=${INSTALL_STAGE1_APTGET-yes}
INSTALL_STAGE2_DPDK=${INSTALL_STAGE2_DPDK-yes}
INSTALL_STAGE3_PROTOBUF=${INSTALL_STAGE3_PROTOBUF-yes}
INSTALL_STAGE4_P4C=${INSTALL_STAGE4_P4C-yes}
INSTALL_STAGE5_T4P4S=${INSTALL_STAGE5_T4P4S-yes}

PROTOBUF_USE_RC=${PROTOBUF_USE_RC-yes}
TODAY=`date +%F`
P4C_COMMIT_DATE=${P4C_COMMIT_DATE-$TODAY}

if [ "$INSTALL_STAGE1_APTGET" != "yes" ] || [ "$INSTALL_STAGE2_DPDK" != "yes" ] || [ "$INSTALL_STAGE3_PROTOBUF" != "yes" ] || [ "$INSTALL_STAGE4_P4C" != "yes" ] || [ "$INSTALL_STAGE5_T4P4S" != "yes" ]; then
    FRESH="no"
    CLEANUP="no"
fi

echo -e "System has $cc`nproc --all`$nn cores; will use $cc$MAX_MAKE_JOBS$nn jobs"
echo Requesting root access...
sudo echo -n ""
echo Root access granted, starting...

if [ "$FRESH" == "yes" ]; then
    unset PROTOBUF_TAG
    unset DPDK_VSN
    unset RTE_SDK
    unset RTE_TARGET
    unset P4C
fi

if [ "$CLEANUP" == "yes" ]; then
    echo Cleaning previously downloaded files and directories
    sudo rm -rf dpdk*
    sudo rm -rf protobuf
    sudo rm -rf p4c
    sudo rm -rf t4p4s*
    sudo rm -f t4p4s_environment_variables.sh
fi

INST_SIZE_DPDK=420
INST_SIZE_PROTOBUF=1500
INST_SIZE_P4C=1700
INST_SIZE_T4P4S_EXAMPLES=4000

APPROX_INSTALL_MB=0
[ $INSTALL_STAGE2_DPDK == "yes" ] && APPROX_INSTALL_MB=$(($APPROX_INSTALL_MB+INST_SIZE_DPDK))
[ $INSTALL_STAGE3_PROTOBUF == "yes" ] && APPROX_INSTALL_MB=$(($APPROX_INSTALL_MB+INST_SIZE_PROTOBUF))
[ $INSTALL_STAGE4_P4C == "yes" ] && APPROX_INSTALL_MB=$(($APPROX_INSTALL_MB+INST_SIZE_P4C))

APPROX_TOTAL_MB=$(($APPROX_INSTALL_MB+INST_SIZE_T4P4S_EXAMPLES))
FREE_MB="`df --output=avail -m . | tail -1 | tr -d '[:space:]'`"

SKIP_CHECK=${SKIP_CHECK-no}

if [ "$SKIP_CHECK" == "no" ] && [ "$FREE_MB" -lt "$APPROX_INSTALL_MB" ]; then
    echo -e "Bootstrapping requires approximately $cc$APPROX_INSTALL_MB MB$nn of free space"
    echo -e "You seem to have $cc$FREE_MB MB$nn of free space on the current drive"
    echo -e "To force installation, run ${cc}SKIP_CHECK=1 $0$nn"
    exit
else
    echo -e "Installation will use approximately $cc$APPROX_INSTALL_MB MB$nn of space"
fi

if [ "$FREE_MB" -lt "$APPROX_TOTAL_MB" ]; then
    echo -e "Bootstrapping and then compiling all examples requires approximately $cc$APPROX_TOTAL_MB MB$nn of free space"
    echo -e "${cc}Warning$nn: you seem to have $cc$FREE_MB MB$nn of free space on the current drive"
fi


if [ ! `which curl` ] || [ ! `which git` ]; then
    echo -e "Installing ${cc}curl$nn and ${cc}git$nn"
    sudo apt-get -y install curl git
fi

if [ "$INSTALL_STAGE3_PROTOBUF" == "yes" ]; then
    if [ "$PROTOBUF_TAG" == "" ]; then
        [ "$PROTOBUF_USE_RC" == "yes" ] && NEWEST_PROTOBUF_TAG=`git ls-remote --refs --tags https://github.com/google/protobuf | grep -ve "[-]rc" | tail -1 | cut -f3 -d'/'`
        [ "$PROTOBUF_USE_RC" != "yes" ] && NEWEST_PROTOBUF_TAG=`git ls-remote --refs --tags https://github.com/google/protobuf                    | tail -1 | cut -f3 -d'/'`
    fi
    PROTOBUF_TAG=${PROTOBUF_TAG-$NEWEST_PROTOBUF_TAG}

    echo -e "Using ${cc}protobuf$nn tag $cc$PROTOBUF_TAG$nn"
fi

if [ "$INSTALL_STAGE2_DPDK" == "yes" ]; then
    if [ "$DPDK_VSN" != "" ]; then
        echo -e "Using ${cc}user set DPDK version$nn \$DPDK_VSN=$cc${DPDK_VSN}$nn"
    else
        # Get the most recent DPDK version
        vsn=`curl -s "https://fast.dpdk.org/rel/" --list-only \
            | grep ".tar.xz" \
            | sed -e "s/^[^>]*>dpdk-\([0-9.]*\)\.tar\.xz[^0-9]*\([0-9]\{2\}\)-\([a-zA-Z]\{3\}\)-\([0-9]\{4\}\) \([0-9]\{2\}\):\([0-9]\{2\}\).*$/\4 \3 \2 \5 \6 \1/g" \
            | sed -e "s/ \([0-9]\{2\}\)[.]\([0-9]\{2\}\)$/ \1.\2.-1/g" \
            | tr '.' ' ' \
            | sort -k6,6n -k7,7n -k8,8n -k1,1 -k2,2M -k3,3 -k4,4 -k5,5 \
            | tac \
            | cut -d" " -f 6- \
            | sed -e "s/^\([0-9\-]*\) \([0-9\-]*\) \([0-9\-]*\)$/\3 \1.\2/g" \
            | uniq -f1 \
            | head -1`

        vsn=($vsn)

        DPDK_NEWEST_VSN="${vsn[1]}"
        DPDK_VSN=${DPDK_VSN-$DPDK_NEWEST_VSN}
        echo -e "Using DPDK version $cc${DPDK_VSN}$nn"
    fi

    DPDK_FILEVSN="$DPDK_VSN"
    [ "${vsn[0]}" != "-1" ] && DPDK_FILEVSN="$DPDK_VSN.${vsn[0]}"
fi


if [ "$RTE_TARGET" != "" ]; then
    echo -e "Using ${cc}DPDK target$nn RTE_TARGET=$cc$RTE_TARGET$nn"
else
    T4P4S_CC=${T4P4S_CC-gcc}
    which clang >/dev/null
    [ $? -eq 0 ] && T4P4S_CC=clang

    echo -e "DPDK will be compiled using ${cc}$T4P4S_CC$nn"
    export RTE_TARGET=${RTE_TARGET-"x86_64-native-linuxapp-$T4P4S_CC"}
fi

T4P4S_LD=${T4P4S_LD-lld}


echo -e "Using ${cc}p4c$nn commit from ${cc}$P4C_COMMIT_DATE$nn"


NEWEST_PYTHON=`apt-cache search python3 | grep -e "^python3.[0-9]* " | cut -f 1 -d " " | sort -t "." -k 2,2nr | head -1`
PYTHON3=${PYTHON3-$NEWEST_PYTHON}
[ "$NEWEST_PYTHON" == "$PYTHON3" ] && echo -e "Using the newest Python 3 version ${cc}${PYTHON3}$nn"
[ "$NEWEST_PYTHON" != "$PYTHON3" ] && echo -e "Using Python 3 version ${cc}${PYTHON3}$nn (the newest one available is ${cc}${NEWEST_PYTHON}$nn)"

PKGS_PYTHON="${PYTHON3} ${PYTHON3}-dev python3-scapy python3-ipaddr python3-dill python3-setuptools"
PKGS_LIB="libtool libgc-dev libprotobuf-dev libprotoc-dev libnuma-dev libfl-dev libgmp-dev libboost-dev libboost-iostreams-dev"
PKGS_MAKE="meson ninja-build automake bison flex cmake ccache lld pkg-config"
REQUIRED_PACKAGES="$PKGS_PYTHON $PKGS_LIB $PKGS_MAKE g++ tcpdump"
if [ "$USE_OPTIONAL_PACKAGES" == "yes" ]; then
    OPT_PACKAGES="python3-ipdb python3-termcolor python3-colored python3-pip python3-yaml python3-ujson python3-ruamel.yaml gnome-terminal"
fi

T4P4S_DIR=${T4P4S_DIR-t4p4s}
[ $# -gt 0 ] && T4P4S_DIR="t4p4s-$1" && T4P4S_CLONE_OPT="$T4P4S_DIR -b $1" && echo -e "Using the $cc$1$nn branch of T4P4S"


echo

# Download libraries
if [ "$INSTALL_STAGE1_APTGET" == "yes" ]; then
    sudo apt-get update && sudo apt-get -y install $REQUIRED_PACKAGES $OPT_PACKAGES &
    WAITPROC_APTGET="$!"
    [ "$PARALLEL_INSTALL" != "yes" ] && wait "$WAITPROC_APTGET"
fi

if [ "$INSTALL_STAGE2_DPDK" == "yes" ]; then
    [ ! -d "dpdk-${DPDK_VSN}" ] && wget -q -o /dev/null http://fast.dpdk.org/rel/dpdk-$DPDK_FILEVSN.tar.xz && tar xJf dpdk-$DPDK_FILEVSN.tar.xz && rm dpdk-$DPDK_FILEVSN.tar.xz &
    WAITPROC_DPDK="$!"
    [ "$PARALLEL_INSTALL" != "yes" ] && wait "$WAITPROC_DPDK"
fi

if [ "$INSTALL_STAGE3_PROTOBUF" == "yes" ]; then
    [ ! -d "protobuf" ] && git clone --recursive -b "${PROTOBUF_TAG}" https://github.com/google/protobuf &
    WAITPROC_PROTOBUF="$!"
    [ "$PARALLEL_INSTALL" != "yes" ] && wait "$WAITPROC_PROTOBUF"
fi

if [ "$INSTALL_STAGE4_P4C" == "yes" ]; then
    [ ! -d "p4c" ] && git clone --recursive https://github.com/p4lang/p4c && cd p4c && git checkout `git rev-list -1 --before="$P4C_COMMIT_DATE" master` && git submodule update --init --recursive &
    WAITPROC_P4C="$!"
    [ "$PARALLEL_INSTALL" != "yes" ] && wait "$WAITPROC_P4C"
fi

if [ "$INSTALL_STAGE5_T4P4S" == "yes" ]; then
    [ ! -d t4p4s ] && git clone --recursive https://github.com/P4ELTE/t4p4s $T4P4S_CLONE_OPT &
    WAITPROC_T4P4S="$!"
    [ "$PARALLEL_INSTALL" != "yes" ] && wait "$WAITPROC_T4P4S"
fi


ctrl_c_handler() {
    kill -9 $WAITPROC_APTGET 2>/dev/null
    kill -9 $WAITPROC_DPDK 2>/dev/null
    kill -9 $WAITPROC_PROTOBUF 2>/dev/null
    kill -9 $WAITPROC_P4C 2>/dev/null
    kill -9 $WAITPROC_T4P4S 2>/dev/null

    echo "Ctrl-C pressed, exiting"
    exit
}

trap 'ctrl_c_handler' INT


if [ "$INSTALL_STAGE1_APTGET" == "yes" ]; then
    [ "$PARALLEL_INSTALL" == "yes" ] && wait "$WAITPROC_APTGET"

    ${PYTHON3} -m pip install pybind11
    ${PYTHON3} -m pip install pysimdjson

    if [ "$USE_OPTIONAL_PACKAGES" == "yes" ]; then
        ${PYTHON3} -m pip install backtrace
    fi
fi

if [ "$INSTALL_STAGE2_DPDK" == "yes" ]; then
    [ "$PARALLEL_INSTALL" == "yes" ] && wait "$WAITPROC_DPDK"

    export RTE_SDK=`pwd`/`ls -d dpdk*$DPDK_FILEVSN*/`

    cd "$RTE_SDK"
    CC=${T4P4S_CC} CC_LD=${T4P4S_LD} $MESON build
    $NINJA -C build
    sudo $NINJA -C build install
    sudo ldconfig
    cd "$WORKDIR"
fi

if [ "$INSTALL_STAGE3_PROTOBUF" == "yes" ]; then
    [ "$PARALLEL_INSTALL" == "yes" ] && wait "$WAITPROC_PROTOBUF"

    cd protobuf
    ./autogen.sh
    ./configure CC=${T4P4S_CC} LD=${T4P4S_LD}
    sudo make install -j ${MAX_MAKE_JOBS}
    sudo ldconfig
    cd "$WORKDIR"
fi

if [ "$INSTALL_STAGE4_P4C" == "yes" ]; then
    [ "$PARALLEL_INSTALL" == "yes" ] && wait "$WAITPROC_P4C"

    export P4C=`pwd`/p4c

    cd p4c
    ./bootstrap.sh
    cd build
    CC=$T4P4S_CC LD=$T4P4S_LD cmake .. -DENABLE_BMV2=OFF -DENABLE_EBPF=OFF -DENABLE_P4C_GRAPHS=OFF -DENABLE_GTESTS=OFF
    sudo make install -j ${MAX_MAKE_JOBS}
    cd "$WORKDIR"
fi

# Save environment config
if [ "$INSTALL_STAGE5_T4P4S" == "yes" ]; then
    [ "$PARALLEL_INSTALL" == "yes" ] && wait "$WAITPROC_T4P4S"

    cat <<EOF >./t4p4s_environment_variables.sh
export DPDK_VSN=${DPDK_VSN}
export RTE_SDK=`pwd`/`ls -d dpdk*$DPDK_FILEVSN*/`
export RTE_TARGET=${RTE_TARGET}
export P4C=`pwd`/p4c
export T4P4S=${T4P4S_DIR}
EOF

    chmod +x `pwd`/t4p4s_environment_variables.sh
    . `pwd`/t4p4s_environment_variables.sh

    echo Environment variable config is done
    echo -e "Environment variable config is saved in ${cc}`pwd`/t4p4s_environment_variables.sh$nn"

    # remove all lines containing "t4p4s_environment_variables"
    sed -ni '/t4p4s_environment_variables/!p' ~/.profile

    echo >> ~/.profile
    echo ". `pwd`/t4p4s_environment_variables.sh" >> ~/.profile
    echo -e "Environment variable config is ${cc}enabled on login$nn: your ${cc}~/.profile$nn will run `pwd`/t4p4s_environment_variables.sh"
fi
