
# Highlight colours
cc="\033[1;33m"     # yellow
nn="\033[0m"

if [ $# == 1 ] && [ "$1" == "showenvs" ]; then
    escape_char=$(printf '\033')
    colours=([0]="${escape_char}[1;32m" [1]="${escape_char}[1;33m" [2]="${escape_char}[1;31m")
    nn="${escape_char}[0m"

    echo "The ${colours[0]}$0$nn script uses the following ${colours[1]}default values$nn for ${colours[0]}environment variables$nn."
    cat "$0" | grep -e '\([A-Z0-9_]*\)=[$][{]\1-'| sed "s/[ ]*export//" | sed "s/[ ]*\([^=]*\)=[$][{]\1-\(.*\)[}]$/    ${colours[0]}\1$nn=${colours[1]}\2$nn/" | sort
    echo "Override them like this to customise the script's behaviour."
    echo "    ${colours[0]}MAX_MAKE_JOBS$nn=${colours[1]}8$nn ${colours[0]}T4P4S_CC$nn=${colours[1]}clang$nn ${colours[0]}$0$nn"
    exit
fi

APPROX_INSTALL_MB="3500"
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

MAX_MAKE_JOBS=${MAX_MAKE_JOBS-`nproc --all`}
FRESH=${FRESH-no}
CLEANUP=${CLEANUP-no}
USE_OPTIONAL_PACKAGES=${USE_OPTIONAL_PACKAGES-yes}

echo -e "System has $cc`nproc --all`$nn cores; will use $cc$MAX_MAKE_JOBS$nn jobs"
echo Requesting root access...
sudo echo -n ""
echo Root access granted, starting...

if [ "$FRESH" == "1" ]; then
    CLEANUP=1
    unset PROTOBUF_BRANCH
    unset DPDK_VSN
    unset RTE_SDK
    unset RTE_TARGET
    unset P4C
fi

if [ "$CLEANUP" == "1" ]; then
    echo Cleaning previously downloaded files and directories
    sudo rm -rf dpdk*
    sudo rm -rf protobuf
    sudo rm -rf p4c
    sudo rm -rf t4p4s*
    sudo rm -f t4p4s_environment_variables.sh
fi

if [ ! `which curl` ] || [ ! `which git` ]; then
    echo -e "Installing ${cc}curl$nn and ${cc}git$nn"
    sudo apt-get -y install curl git
fi

NEWEST_PROTOBUF_BRANCH=`git ls-remote --refs --tags https://github.com/google/protobuf | tail -1 | cut -f3 -d'/'`

# Set sensible defaults
export PARALLEL_INSTALL=${PARALLEL_INSTALL-yes}
export PROTOBUF_BRANCH=${PROTOBUF_BRANCH-$NEWEST_PROTOBUF_BRANCH}

echo -e "Using ${cc}protobuf$nn branch $cc$PROTOBUF_BRANCH$nn"

# Note: currently unused, this variable can pin T4P4S bootstrap on a certain p4c commit
# P4C_COMMIT=${P4C_COMMIT-80f8970b5ec8e57c4a3611da343461b5b0a8dda3}

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


if [ "$RTE_TARGET" != "" ]; then
    echo -e "Using ${cc}DPDK target$nn RTE_TARGET=$cc$RTE_TARGET$nn"
else
    T4P4S_CC=${T4P4S_CC-gcc}
    which clang >/dev/null
    [ $? -eq 0 ] && T4P4S_CC=clang

    echo -e "DPDK will be compiled using ${cc}$T4P4S_CC$nn"
    export RTE_TARGET=${RTE_TARGET-"x86_64-native-linuxapp-$T4P4S_CC"}
fi

T4P4S_LD=${T4P4S_LD-ld.lld}


NEWEST_PYTHON=`apt-cache search python3 | grep -e "^python3.[0-9]* " | cut -f 1 -d " " | sort -t "." -k 2,2nr | head -1`
PYTHON3=${PYTHON3-$NEWEST_PYTHON}
[ "$NEWEST_PYTHON" == "$PYTHON3" ] && echo -e "Using the newest Python 3 version ${cc}${PYTHON3}$nn"
[ "$NEWEST_PYTHON" != "$PYTHON3" ] && echo -e "Using Python 3 version ${cc}${PYTHON3}$nn (the newest one available is ${cc}${NEWEST_PYTHON}$nn)"

PKGS_PYTHON="${PYTHON3} python3-scapy python3-ipaddr python3-dill python3-setuptools python3-dev"
PKGS_LIB="libtool libgc-dev libprotobuf-dev libnuma-dev libfl-dev libgmp-dev libboost-dev libboost-iostreams-dev"
PKGS_MAKE="automake bison flex cmake ccache lld pkg-config"
REQUIRED_PACKAGES="$PKGS_PYTHON $PKGS_LIB $PKGS_MAKE g++ tcpdump"
if [ "$USE_OPTIONAL_PACKAGES" == "yes" ]; then
    OPT_PACKAGES="python3-ipdb python3-termcolor python3-pip python3-yaml python3-ujson python3-ruamel.yaml gnome-terminal"
fi

T4P4S_DIR=${T4P4S_DIR-t4p4s}
[ $# -gt 0 ] && T4P4S_DIR="t4p4s-$1" && T4P4S_CLONE_OPT="$T4P4S_DIR -b $1" && echo -e "Using the $cc$1$nn branch of T4P4S"


echo

# Download libraries
sudo apt-get update && sudo apt-get -y install $REQUIRED_PACKAGES $OPT_PACKAGES &
WAITPROC_APTGET="$!"
[ "$PARALLEL_INSTALL" == "yes" ] || wait "$WAITPROC_APTGET"

[ ! -d "dpdk-${DPDK_VSN}" ] && wget -q -o /dev/null http://fast.dpdk.org/rel/dpdk-$DPDK_FILEVSN.tar.xz && tar xJf dpdk-$DPDK_FILEVSN.tar.xz && rm dpdk-$DPDK_FILEVSN.tar.xz &
WAITPROC_DPDK="$!"
[ "$PARALLEL_INSTALL" == "yes" ] || wait "$WAITPROC_DPDK"

[ ! -d "protobuf" ] && git clone --recursive -b "${PROTOBUF_BRANCH}" https://github.com/google/protobuf &
WAITPROC_PROTOBUF="$!"
[ "$PARALLEL_INSTALL" == "yes" ] || wait "$WAITPROC_PROTOBUF"

[ ! -d "p4c" ] && git clone --recursive https://github.com/p4lang/p4c && cd p4c && git checkout $P4C_COMMIT && git submodule update --init --recursive &
WAITPROC_P4C="$!"
[ "$PARALLEL_INSTALL" == "yes" ] || wait "$WAITPROC_P4C"

[ ! -d t4p4s ] && git clone --recursive https://github.com/P4ELTE/t4p4s $T4P4S_CLONE_OPT &
WAITPROC_T4P4S="$!"
[ "$PARALLEL_INSTALL" == "yes" ] || wait "$WAITPROC_T4P4S"



# Wait for apt-get to finish
[ "$PARALLEL_INSTALL" == "yes" ] || wait "$WAITPROC_APTGET"

${PYTHON3} -m pip install pybind11
${PYTHON3} -m pip install pysimdjson

if [ "$USE_OPTIONAL_PACKAGES" == "yes" ]; then
    ${PYTHON3} -m pip install backtrace
fi


# Setup DPDK
[ "$PARALLEL_INSTALL" == "yes" ] || wait "$WAITPROC_DPDK"

export RTE_SDK=`pwd`/`ls -d dpdk*$DPDK_FILEVSN*/`

cd "$RTE_SDK"
MAKE_PAUSE=n make install DESTDIR="${RTE_TARGET}" T="${RTE_TARGET}" LDFLAGS="-fuse-ld=${T4P4S_LD}" -j ${MAX_MAKE_JOBS}
cd ..


# Setup protobuf
[ "$PARALLEL_INSTALL" == "yes" ] || wait "$WAITPROC_PROTOBUF"

cd protobuf
./autogen.sh
./configure LD=${T4P4S_LD}
make -j ${MAX_MAKE_JOBS}
sudo make install -j ${MAX_MAKE_JOBS}
sudo ldconfig
cd ..


# Setup p4c
[ "$PARALLEL_INSTALL" == "yes" ] || wait "$WAITPROC_P4C"

export P4C=`pwd`/p4c

cd p4c
./bootstrap.sh
cd build
LD=ld.gold cmake ..
make -j ${MAX_MAKE_JOBS}
sudo make install -j ${MAX_MAKE_JOBS}
cd ../..


# Enter t4p4s directory
[ "$PARALLEL_INSTALL" == "yes" ] || wait "$WAITPROC_T4P4S"

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

if [[ $(grep "t4p4s_environment_variables.sh" ~/.profile) ]]; then
    echo -e "Your ${cc}~/.profile$nn is ${cc}not modified$nn, as it already calls t4p4s_environment_variables.sh"
else
    echo >> ~/.profile
    echo ". `pwd`/t4p4s_environment_variables.sh" >> ~/.profile
    echo -e "Environment variable config is ${cc}enabled on login$nn: your ${cc}~/.profile$nn will run `pwd`/t4p4s_environment_variables.sh"
fi

cd ${T4P4S_DIR}
