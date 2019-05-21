#!/bin/bash

# Highlight colours
cc="\033[1;33m"     # yellow
nn="\033[0m"

MAX_MAKE_JOBS=${MAX_MAKE_JOBS-`nproc --all`}

echo -e "System has $cc`nproc --all`$nn cores; will use $cc$MAX_MAKE_JOBS$nn jobs"
echo Requesting root access...
sudo echo -n ""
echo Root access granted, starting...

# Set sensible defaults
export PARALLEL_INSTALL=${PARALLEL_INSTALL-1}
export PROTOBUF_BRANCH=${PROTOBUF_BRANCH-$(git ls-remote --tags https://github.com/google/protobuf | tail -1 | cut -f3 -d'/')}

which clang >/dev/null
if [ $? -eq 0 ]; then
    [[ "$RTE_TARGET" == "" ]] && echo -e "DPDK will be compiled using ${cc}clang$nn"
    export RTE_TARGET=${RTE_TARGET-x86_64-native-linuxapp-clang}
else
    [[ "$RTE_TARGET" == "" ]] && echo -e "DPDK will be compiled using ${cc}gcc$nn"
    export RTE_TARGET=${RTE_TARGET-x86_64-native-linuxapp-gcc}
fi

# Note: recent versions of P4C introduced changes currently incompatible with T4P4S
# P4C_COMMIT=${P4C_COMMIT-80f8970b5ec8e57c4a3611da343461b5b0a8dda3}

echo Determining newest DPDK version...

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

DPDK_VSN=$(echo $vsn | cut -d ' ' -f 2)
DPDK_FILEVSN="$DPDK_VSN"
[ "$(echo $vsn | cut -d ' ' -f 1)" != "-1" ] && DPDK_FILEVSN="$DPDK_VSN.$(echo $vsn | cut -d ' ' -f 1)"

echo -e "Using DPDK version $cc${DPDK_VSN}$nn"

echo

# Download libraries
echo ========== DOWNLOADING DEPENDENCIES ==========
sudo apt-get update && sudo apt-get -y install g++ git automake libtool libgc-dev bison flex libfl-dev libgmp-dev libboost-dev libboost-iostreams-dev pkg-config python python-scapy python-ipaddr tcpdump cmake python-setuptools libprotobuf-dev libnuma-dev curl

echo ========== DOWNLOADING DPDK ==========
wget http://fast.dpdk.org/rel/dpdk-$DPDK_FILEVSN.tar.xz && tar xJf dpdk-$DPDK_FILEVSN.tar.xz && rm dpdk-$DPDK_FILEVSN.tar.xz

echo ========== CLONING PROTOBUF ==========
git clone --depth=1 --recursive -b "${PROTOBUF_BRANCH}" https://github.com/google/protobuf

echo ========== CLONING P4C ==========
git clone --depth=1 --recursive https://github.com/p4lang/p4c && cd p4c && git checkout $P4C_COMMIT && git submodule update --init --recursive
# git clone --recursive https://github.com/p4lang/p4c && cd p4c && git submodule update --init --recursive

echo ========== CLONING T4P4S ==========
git clone --depth=1 --recursive https://github.com/P4ELTE/t4p4s

echo "export RTE_SDK=$PWD/$(ls -d dpdk*$DPDK_FILEVSN*/)"
export RTE_SDK=$PWD/$(ls -d dpdk*$DPDK_FILEVSN*/)

cd "$RTE_SDK"
echo ========== DOING DPDK ==========
make defconfig
make -j${MAX_MAKE_JOBS}
make install DESTDIR="${RTE_TARGET}" T="${RTE_TARGET}"
cd ..

# Setup protobuf
echo ========== DOING PROTOBUF ==========
cd protobuf
./autogen.sh
./configure
make -j${MAX_MAKE_JOBS}
sudo make install
sudo ldconfig
cd ..


# Setup p4c
export P4C=$PWD/p4c

echo ========== DOING P4C ==========
cd p4c
./bootstrap.sh
cd build
cmake ..
make -j${MAX_MAKE_JOBS}
sudo make install
cd ../..

# Enter t4p4s directory
cat <<EOF >./t4p4s_environment_variables.sh
export DPDK_VSN=$DPDK_VSN
export RTE_SDK=$RTE_SDK
export RTE_TARGET=$RTE_TARGET
export P4C=$PWD/p4c
EOF

chmod +x $PWD/t4p4s_environment_variables.sh
. $PWD/t4p4s_environment_variables.sh

echo Environment variable config is done
echo -e "Environment variable config is saved in ${cc}$PWD/t4p4s_environment_variables.sh$nn"

if [[ $(grep "t4p4s_environment_variables.sh" ~/.profile) ]]; then
    echo -e "Your ${cc}~/.profile$nn is ${cc}not modified$nn, as it already calls t4p4s_environment_variables.sh"
else
    echo >> ~/.profile
    echo ". $PWD/t4p4s_environment_variables.sh" >> ~/.profile
    echo -e "Environment variable config is ${cc}enabled on login$nn: your ${cc}~/.profile$nn will run $PWD/t4p4s_environment_variables.sh"
fi
