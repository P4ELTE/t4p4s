# Set sensible defaults
export PARALLEL_INSTALL=${PARALLEL_INSTALL-1}
export PROTOBUF_BRANCH=${PROTOBUF_BRANCH-v3.4.1}
export RTE_TARGET=${RTE_TARGET-x86_64-native-linuxapp-gcc}

vsns=(18.02 17.11.2 17.08 17.05.2 17.02.1 16.11.3 16.07.2)
for vsn in ${vsns[*]};
do
    shortvsn=`echo $vsn | sed -e 's/\([0-9]*[.][0-9]*\).*/\1/g'`
    if [ "$DPDK_VSN" == $vsn -o "$DPDK_VSN" == $shortvsn ]; then
        export DPDK_FILEVSN=$vsn
        break
    fi
done

export DPDK_FILEVSN=${DPDK_FILEVSN-${vsns[0]}}
shortvsn=`echo $vsn | sed -e 's/\([0-9]*[.][0-9]*\).*/\1/g'`
export DPDK_VSN=${DPDK_VSN-$shortvsn}

# Download libraries
sudo apt-get update && sudo apt-get -y install g++ git automake libtool libgc-dev bison flex libfl-dev libgmp-dev libboost-dev libboost-iostreams-dev pkg-config python python-scapy python-ipaddr tcpdump cmake python-setuptools libprotobuf-dev libnuma-dev curl &
WAITPROC_APTGET="$!"
[ $PARALLEL_INSTALL -ne 0 ] || wait "$WAITPROC_APTGET"

wget http://fast.dpdk.org/rel/dpdk-$DPDK_FILEVSN.tar.xz && tar xJf dpdk-$DPDK_FILEVSN.tar.xz && rm dpdk-$DPDK_FILEVSN.tar.xz &
WAITPROC_DPDK="$!"
[ $PARALLEL_INSTALL -ne 0 ] || wait "$WAITPROC_DPDK"

git clone --recursive -b "${PROTOBUF_BRANCH}" https://github.com/google/protobuf &
WAITPROC_PROTOBUF="$!"
[ $PARALLEL_INSTALL -ne 0 ] || wait "$WAITPROC_PROTOBUF"

git clone --recursive https://github.com/p4lang/p4c &
WAITPROC_P4C="$!"
[ $PARALLEL_INSTALL -ne 0 ] || wait "$WAITPROC_P4C"

[ -d t4p4s-16 ] || git clone --recursive -b t4p4s-16 https://github.com/P4ELTE/t4p4s t4p4s-16 &
WAITPROC_T4P4S="$!"
[ $PARALLEL_INSTALL -ne 0 ] || wait "$WAITPROC_T4P4S"


# Wait for apt-get to finish
[ $PARALLEL_INSTALL -ne 1 ] || wait "$WAITPROC_APTGET"




# Setup DPDK
[ $PARALLEL_INSTALL -ne 1 ] || wait "$WAITPROC_DPDK"

export RTE_SDK=`pwd`/`ls -d dpdk*$DPDK_FILEVSN*/`

cd "$RTE_SDK"
make install DESTDIR="${RTE_TARGET}" T="${RTE_TARGET}" -j4
cd ..


# Setup protobuf
[ $PARALLEL_INSTALL -ne 1 ] || wait "$WAITPROC_PROTOBUF"

cd protobuf
./autogen.sh
./configure
make -j 4
sudo make install -j 4
sudo ldconfig
cd ..


# Setup p4c
[ $PARALLEL_INSTALL -ne 1 ] || wait "$WAITPROC_P4C"

export P4C=`pwd`/p4c

cd p4c
./bootstrap.sh
cd build
cmake ..
make -j 4
sudo make install -j 4
cd ../..


# Enter t4p4s directory
[ $PARALLEL_INSTALL -ne 1 ] || wait "$WAITPROC_T4P4S"

cd t4p4s-16


# Print environment variables
echo "DPDK_VSN=${DPDK_VSN}"
echo "RTE_SDK=${RTE_SDK}"
echo "P4C=${P4C}"
