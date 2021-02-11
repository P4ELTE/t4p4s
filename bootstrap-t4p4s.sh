
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

if [ `cat /etc/os-release | grep ID_LIKE= | cut -d= -f2` == "ubuntu" ]; then
    MIN_UBUNTU_VSN=20

    UBUNTU_VSN=`cat /etc/os-release | grep VERSION_ID= | cut -d'"' -f2`
    [ $UBUNTU_VSN -lt $MIN_UBUNTU_VSN ] && echo -e "${cc}Warning$nn: Ubuntu version lower than minimum supported ($cc$MIN_UBUNTU_VSN$nn), installation may fail" && echo
fi

INST_MB_DPDK=420
INST_MB_PROTOBUF=1500
INST_MB_P4C=1700
INST_MB_GRPC=1400
INST_MB_PI=640
INST_MB_T4P4S_EXAMPLES=4000

P4C_APPROX_KB_PER_JOB=1200


WORKDIR=`pwd`
LOGDIR=`pwd`/log

SYSTEM_THREAD_COUNT=`nproc --all`

MAX_MAKE_JOBS=${MAX_MAKE_JOBS-$SYSTEM_THREAD_COUNT}
FRESH=${FRESH-yes}
CLEANUP=${CLEANUP-yes}
USE_OPTIONAL_PACKAGES=${USE_OPTIONAL_PACKAGES-yes}
PARALLEL_INSTALL=${PARALLEL_INSTALL-yes}

INSTALL_STAGE1_PACKAGES=${INSTALL_STAGE1_PACKAGES-yes}
INSTALL_STAGE2_DPDK=${INSTALL_STAGE2_DPDK-yes}
INSTALL_STAGE3_PROTOBUF=${INSTALL_STAGE3_PROTOBUF-yes}
INSTALL_STAGE4_P4C=${INSTALL_STAGE4_P4C-yes}
INSTALL_STAGE5_GRPC=${INSTALL_STAGE5_GRPC-yes}
INSTALL_STAGE6_T4P4S=${INSTALL_STAGE6_T4P4S-yes}

PROTOBUF_USE_RC=${PROTOBUF_USE_RC-yes}
TODAY=`date +%F`
P4C_COMMIT_DATE=${P4C_COMMIT_DATE-$TODAY}

if [ "$INSTALL_STAGE4_P4C" == "yes" ]; then
    MEM_FREE_KB=`cat /proc/meminfo | grep MemFree | grep -Eo '[0-9]+'`
    JOBS_BY_MEM_FREE=$(($MEM_FREE_KB / $P4C_APPROX_KB_PER_JOB / 1024))
    MAX_MAKE_JOBS_P4C=$(($JOBS_BY_MEM_FREE<$MAX_MAKE_JOBS ? $JOBS_BY_MEM_FREE : $MAX_MAKE_JOBS))
    MAX_MAKE_JOBS_P4C=$(($MAX_MAKE_JOBS_P4C <= 0 ? 1 : $MAX_MAKE_JOBS_P4C))
    echo -e "System has $cc`nproc --all`$nn cores; will use $cc$MAX_MAKE_JOBS$nn jobs in general, $cc$MAX_MAKE_JOBS_P4C$nn for ${cc}p4c$nn compilation"
else
    echo -e "System has $cc`nproc --all`$nn cores; will use $cc$MAX_MAKE_JOBS$nn jobs"
fi

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
    echo "Cleaning previously downloaded files and directories (move to backup folder named cleanup_archive)"

    mkdir -p cleanup_archive

    mv --backup=numbered dpdk* cleanup_archive/
    mv --backup=numbered protobuf cleanup_archive/
    mv --backup=numbered p4c cleanup_archive/
    mv --backup=numbered grpc cleanup_archive/
    mv --backup=numbered PI cleanup_archive/
    mv --backup=numbered P4Runtime_GRPCPP cleanup_archive/
    mv --backup=numbered t4p4s* cleanup_archive/
    mv --backup=numbered t4p4s_environment_variables.sh cleanup_archive/
fi

APPROX_INSTALL_MB=0
[ "$INSTALL_STAGE2_DPDK" == "yes" ] && APPROX_INSTALL_MB=$(($APPROX_INSTALL_MB+INST_MB_DPDK))
[ "$INSTALL_STAGE3_PROTOBUF" == "yes" ] && APPROX_INSTALL_MB=$(($APPROX_INSTALL_MB+INST_MB_PROTOBUF))
[ "$INSTALL_STAGE4_P4C" == "yes" ] && APPROX_INSTALL_MB=$(($APPROX_INSTALL_MB+INST_MB_P4C))
[ "$INSTALL_STAGE5_GRPC" == "yes" ] && APPROX_INSTALL_MB=$(($APPROX_INSTALL_MB+INST_MB_GRPC+INST_MB_PI))

APPROX_TOTAL_MB=$(($APPROX_INSTALL_MB+INST_MB_T4P4S_EXAMPLES))
FREE_MB="`df --output=avail -m . | tail -1 | tr -d '[:space:]'`"

SKIP_CHECK=${SKIP_CHECK-no}


T4P4S_CC=${T4P4S_CC-gcc}
which clang >/dev/null
[ $? -eq 0 ] && T4P4S_CC=clang

T4P4S_CXX=${T4P4S_CXX-g++}
which clang++ >/dev/null
[ $? -eq 0 ] && T4P4S_CXX=clang++

T4P4S_LD=${T4P4S_LD-bfd}
which ld.lld >/dev/null
[ $? -eq 0 ] && T4P4S_LD=lld

echo -e "Using compilers CC=${cc}$T4P4S_CC$nn, CXX=${cc}$T4P4S_CXX$nn, LD=${cc}$T4P4S_LD$nn"


if [ "$SKIP_CHECK" == "no" ] && [ "$FREE_MB" -lt "$APPROX_INSTALL_MB" ]; then
    echo -e "Bootstrapping requires approximately $cc$APPROX_INSTALL_MB MB$nn of free space"
    echo -e "You seem to have $cc$FREE_MB MB$nn of free space on the current drive"
    echo -e "To force installation, run ${cc}SKIP_CHECK=1 . ./bootstrap.sh$nn"
    exit
else
    echo -e "Installation will use approximately $cc$APPROX_INSTALL_MB MB$nn of space"
fi

if [ "$FREE_MB" -lt "$APPROX_TOTAL_MB" ]; then
    echo -e "Bootstrapping and then compiling all examples requires approximately $cc$APPROX_TOTAL_MB MB$nn of free space"
    echo -e "${cc}Warning$nn: you seem to have $cc$FREE_MB MB$nn of free space on the current drive"
fi



function logfile() {
    case "$1" in
        "curl-git")             LOG_STAGE="01";;
        "apt")                  LOG_STAGE="02";;
        "get-dpdk")             LOG_STAGE="03";;
        "get-protobuf")         LOG_STAGE="04";;
        "get-p4c")              LOG_STAGE="05";;
        "get-grpc")             LOG_STAGE="06";;
        "get-PI")               LOG_STAGE="07";;
        "get-P4Runtime_GRPCPP") LOG_STAGE="08";;
        "get-t4p4s")            LOG_STAGE="09";;
        "python3")              LOG_STAGE="10";;
        "dpdk")                 LOG_STAGE="20";;
        "protobuf")             LOG_STAGE="30";;
        "p4c")                  LOG_STAGE="40";;
        "grpc")                 LOG_STAGE="50";;
        "PI")                   LOG_STAGE="51";;
        "P4Runtime_GRPCPP")     LOG_STAGE="52";;
        "t4p4s")                LOG_STAGE="60";;
        *)                      LOG_STAGE="99";;
    esac
    echo "$LOGDIR/${LOG_STAGE}_$1$2.txt"
}

mkdir -p "$LOGDIR"

if [ ! `which curl` ] || [ ! `which git` ]; then
    echo -e "Installing ${cc}curl$nn and ${cc}git$nn"
    sudo apt-get -y install curl git >$(logfile "curl-git") 2>&1
fi

if [ "$INSTALL_STAGE3_PROTOBUF" == "yes" ]; then
    if [ "$PROTOBUF_TAG" == "" ]; then
        [ "$PROTOBUF_USE_RC" != "yes" ] && NEWEST_PROTOBUF_TAG=`git ls-remote --refs --tags https://github.com/google/protobuf | grep -ve "[-]rc" | tail -1 | cut -f3 -d'/'`
        [ "$PROTOBUF_USE_RC" == "yes" ] && NEWEST_PROTOBUF_TAG=`git ls-remote --refs --tags https://github.com/google/protobuf                    | tail -1 | cut -f3 -d'/'`
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
        echo -e "Using ${cc}DPDK$nn version $cc${DPDK_VSN}$nn"
    fi

    DPDK_FILEVSN="$DPDK_VSN"
    [ "${vsn[0]}" != "-1" ] && DPDK_FILEVSN="$DPDK_VSN.${vsn[0]}"
fi


if [ "$RTE_TARGET" != "" ]; then
    echo -e "Using ${cc}DPDK target$nn RTE_TARGET=$cc$RTE_TARGET$nn"
else
    export RTE_TARGET=${RTE_TARGET-"x86_64-native-linuxapp-$T4P4S_CC"}
fi


echo -e "Using ${cc}p4c$nn commit from ${cc}$P4C_COMMIT_DATE$nn"


NEWEST_PYTHON=`apt-cache search python3 | grep -e "^python3.[0-9]* " | cut -f 1 -d " " | sort -t "." -k 2,2nr | head -1`
PYTHON3=${PYTHON3-$NEWEST_PYTHON}
[ "$PYTHON3" == "" ] && echo -e "Could not find appropriate Python 3 version, exiting" && exit 1
[ "$NEWEST_PYTHON" == "$PYTHON3" ] && echo -e "Using the newest Python 3 version ${cc}${PYTHON3}$nn"
[ "$NEWEST_PYTHON" != "$PYTHON3" ] && echo -e "Using Python 3 version ${cc}${PYTHON3}$nn (the newest one available is ${cc}${NEWEST_PYTHON}$nn)"

PKGS_PYTHON="${PYTHON3} ${PYTHON3}-dev python3-scapy python3-ipaddr python3-dill python3-setuptools"
PKGS_LIB="libtool libgc-dev libprotobuf-dev libprotoc-dev libnuma-dev libfl-dev libgmp-dev libboost-dev libboost-iostreams-dev"
PKGS_MAKE="meson ninja-build automake bison flex cmake ccache lld pkg-config"
PKGS_GRPC=""
[ "$INSTALL_STAGE5_GRPC" == "yes" ] && PKGS_GRPC="libjudy-dev libssl-dev libboost-thread-dev libboost-dev libboost-system-dev libboost-thread-dev libtool-bin"
REQUIRED_PACKAGES="$PKGS_PYTHON $PKGS_LIB $PKGS_MAKE $PKGS_GRPC g++ tcpdump"
PIP_PACKAGES="pybind11 pysimdjson"
if [ "$USE_OPTIONAL_PACKAGES" == "yes" ]; then
    OPT_PACKAGES="python3-ipdb python3-termcolor python3-colored python3-pip python3-yaml python3-ujson python3-ruamel.yaml gnome-terminal"
    PIP_PACKAGES="$PIP_PACKAGES backtrace"
fi

T4P4S_DIR=${T4P4S_DIR-t4p4s}
[ $# -gt 0 ] && T4P4S_DIR="t4p4s-$1" && T4P4S_CLONE_OPT="$T4P4S_DIR -b $1" && echo -e "Using the $cc$1$nn branch of T4P4S"


REPO_PATH_protobuf=${REPO_PATH_protobuf-"https://github.com/google/protobuf"}
REPO_PATH_p4c=${REPO_PATH_p4c-"https://github.com/p4lang/p4c"}
REPO_PATH_grpc=${REPO_PATH_grpc-"https://github.com/grpc/grpc"}
REPO_PATH_PI=${REPO_PATH_PI-"https://github.com/p4lang/PI"}
REPO_PATH_P4Runtime_GRPCPP=${REPO_PATH_P4Runtime_GRPCPP-"https://github.com/P4ELTE/P4Runtime_GRPCPP"}
REPO_PATH_t4p4s=${REPO_PATH_t4p4s-"https://github.com/P4ELTE/t4p4s"}

LOCAL_REPO_CACHE=${LOCAL_REPO_CACHE-}

if [ "$LOCAL_REPO_CACHE" != "" ]; then
    [ ! -d "$LOCAL_REPO_CACHE" ] && echo -e "The local repo cache ${cc}$LOCAL_REPO_CACHE$nn is not a directory" && exit 1

    echo -e "Using local repo cache $cc$LOCAL_REPO_CACHE$nn"

    [ ! -d "$LOCAL_REPO_CACHE/protobuf" ] && echo -e "${cc}Warning$nn: \$LOCAL_REPO_CACHE/protobuf$nn is not a directory, installation will use the remote repo"
    [   -d "$LOCAL_REPO_CACHE/protobuf" ] && REPO_PATH_protobuf="$LOCAL_REPO_CACHE/protobuf"

    [ ! -d "$LOCAL_REPO_CACHE/p4c" ] && echo -e "${cc}Warning$nn: \$LOCAL_REPO_CACHE/p4c$nn is not a directory, installation will use the remote repo"
    [   -d "$LOCAL_REPO_CACHE/p4c" ] && REPO_PATH_p4c="$LOCAL_REPO_CACHE/p4c"

    [ ! -d "$LOCAL_REPO_CACHE/grpc" ] && echo -e "${cc}Warning$nn: \$LOCAL_REPO_CACHE/grpc$nn is not a directory, installation will use the remote repo"
    [   -d "$LOCAL_REPO_CACHE/grpc" ] && REPO_PATH_grpc="$LOCAL_REPO_CACHE/grpc"

    [ ! -d "$LOCAL_REPO_CACHE/PI" ] && echo -e "${cc}Warning$nn: \$LOCAL_REPO_CACHE/PI$nn is not a directory, installation will use the remote repo"
    [   -d "$LOCAL_REPO_CACHE/PI" ] && REPO_PATH_PI="$LOCAL_REPO_CACHE/PI"

    [ ! -d "$LOCAL_REPO_CACHE/P4Runtime_GRPCPP" ] && echo -e "${cc}Warning$nn: \$LOCAL_REPO_CACHE/P4Runtime_GRPCPP$nn is not a directory, installation will use the remote repo"
    [   -d "$LOCAL_REPO_CACHE/P4Runtime_GRPCPP" ] && REPO_PATH_P4Runtime_GRPCPP="$LOCAL_REPO_CACHE/P4Runtime_GRPCPP"

    [ ! -d "$LOCAL_REPO_CACHE/t4p4s" ] && echo -e "${cc}Warning$nn: \$LOCAL_REPO_CACHE/t4p4s$nn is not a directory, installation will use the remote repo"
    [   -d "$LOCAL_REPO_CACHE/t4p4s" ] && REPO_PATH_t4p4s="$LOCAL_REPO_CACHE/t4p4s"
fi


echo -e "-------- Configuration done, installing (see details under ${cc}$LOGDIR$nn)"
echo -e "-------- Installation may take several minutes for each of the following libraries"

# Download libraries
if [ "$INSTALL_STAGE1_PACKAGES" == "yes" ]; then
    sudo apt-get update >$(logfile "apt") 2>&1 && sudo apt-get -y install $REQUIRED_PACKAGES $OPT_PACKAGES >>$(logfile "apt") 2>&1 &
    WAITPROC_APTGET="$!"
    [ "$PARALLEL_INSTALL" != "yes" ] && wait "$WAITPROC_APTGET" >/dev/null 2>&1
fi

if [ "$INSTALL_STAGE2_DPDK" == "yes" ]; then
    [ ! -d "dpdk-${DPDK_VSN}" ] && wget -q -o /dev/null http://fast.dpdk.org/rel/dpdk-$DPDK_FILEVSN.tar.xz >$(logfile "get-dpdk") 2>&1 && tar xJf dpdk-$DPDK_FILEVSN.tar.xz && rm dpdk-$DPDK_FILEVSN.tar.xz &
    WAITPROC_DPDK="$!"
    [ "$PARALLEL_INSTALL" != "yes" ] && wait "$WAITPROC_DPDK" >/dev/null 2>&1
fi

if [ "$INSTALL_STAGE3_PROTOBUF" == "yes" ]; then
    [ ! -d "protobuf" ] && git clone "$REPO_PATH_protobuf" --no-hardlinks --recursive -b "${PROTOBUF_TAG}" >$(logfile "get-protobuf") 2>&1 &
    WAITPROC_PROTOBUF="$!"
    [ "$PARALLEL_INSTALL" != "yes" ] && wait "$WAITPROC_PROTOBUF" >/dev/null 2>&1
fi

if [ "$INSTALL_STAGE4_P4C" == "yes" ]; then
    [ ! -d "p4c" ] && git clone "$REPO_PATH_p4c" --no-hardlinks --recursive >$(logfile "get-p4c") 2>&1 && cd p4c && git checkout `git rev-list -1 --before="$P4C_COMMIT_DATE" master` >>$(logfile "get-p4c") 2>&1 && git submodule update --init --recursive &
    WAITPROC_P4C="$!"
    [ "$PARALLEL_INSTALL" != "yes" ] && wait "$WAITPROC_P4C" >/dev/null 2>&1
fi

if [ "$INSTALL_STAGE5_GRPC" == "yes" ]; then
    [ ! -d grpc ] && git clone "$REPO_PATH_grpc" --no-hardlinks --recursive >$(logfile "get-grpc") 2>&1 &
    WAITPROC_GRPC="$!"
    [ "$PARALLEL_INSTALL" != "yes" ] && wait "$WAITPROC_GRPC" >/dev/null 2>&1

    [ ! -d PI ] && git clone "$REPO_PATH_PI" --no-hardlinks --recursive >$(logfile "get-PI") 2>&1 &
    WAITPROC_PI="$!"
    [ "$PARALLEL_INSTALL" != "yes" ] && wait "$WAITPROC_PI" >/dev/null 2>&1

    [ ! -d P4Runtime_GRPCPP ] && git clone "$REPO_PATH_P4Runtime_GRPCPP" --no-hardlinks --recursive >$(logfile "get-P4Runtime_GRPCPP") 2>&1 &
    WAITPROC_P4Runtime_GRPCPP="$!"
    [ "$PARALLEL_INSTALL" != "yes" ] && wait "$WAITPROC_P4Runtime_GRPCPP" >/dev/null 2>&1
fi

if [ "$INSTALL_STAGE6_T4P4S" == "yes" ]; then
    [ ! -d t4p4s ] && git clone "$REPO_PATH_t4p4s" --no-hardlinks --recursive $T4P4S_CLONE_OPT >$(logfile "get-t4p4s") 2>&1 &
    WAITPROC_T4P4S="$!"
    [ "$PARALLEL_INSTALL" != "yes" ] && wait "$WAITPROC_T4P4S" >/dev/null 2>&1
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


if [ "$INSTALL_STAGE1_PACKAGES" == "yes" ]; then
    [ "$PARALLEL_INSTALL" == "yes" ] && wait "$WAITPROC_APTGET" >/dev/null 2>&1

    echo -e "Installing ${cc}Python 3 packages$nn"

    ${PYTHON3} -m pip install $PIP_PACKAGES >$(logfile "python3") 2>&1

    # reinstall meson if necessary
    MESON_NEEDS_REINSTALL=0
    MESON_VSN=`meson --version 2>/dev/null`
    MESON_ERRCODE=$?
    [ ${MESON_ERRCODE} -ne 0 ] && echo "There is a problem with executing ${cc}meson$nn (error code ${MESON_ERRCODE})" && MESON_NEEDS_REINSTALL=1

    if [ ${MESON_ERRCODE} -eq 0 ]; then
        MIN_REQ_MESON_VSN=0.53
        MESON_MIN_VSN=`echo -e "${MIN_REQ_MESON_VSN}\n${MESON_VSN}" | sort -t '.' -k 1,1 -k 2,2 | head -1`
        [ "$MESON_MIN_VSN" != "$MIN_REQ_MESON_VSN" ] && echo "Current ${cc}meson$nn version ${cc}${MESON_VSN}$nn is older than the required ${cc}${MIN_REQ_MESON_VSN}$nn" && MESON_NEEDS_REINSTALL=1
    fi

    if [ "$MESON_NEEDS_REINSTALL" == 1 ]; then
        echo -e "Reinstalling ${cc}meson$nn"
        sudo ${PYTHON3} -m pip install meson >$(logfile "python3" ".meson") 2>&1
    fi
fi

if [ "$INSTALL_STAGE2_DPDK" == "yes" ]; then
    [ "$PARALLEL_INSTALL" == "yes" ] && wait "$WAITPROC_DPDK" >/dev/null 2>&1

    echo -e "Setting up ${cc}DPDK$nn"

    export RTE_SDK=`pwd`/`ls -d dpdk*$DPDK_FILEVSN*/`

    cd "$RTE_SDK"
    CC=${T4P4S_CC} CC_LD=${T4P4S_LD} meson build -Dtests=false >$(logfile "dpdk") 2>&1
    ninja -C build >>$(logfile "dpdk" ".ninja") 2>&1
    sudo ninja -C build install >>$(logfile "dpdk" ".ninja.install") 2>&1
    sudo ldconfig
    cd "$WORKDIR"
fi


if [ "$INSTALL_STAGE3_PROTOBUF" == "yes" ]; then
    [ "$PARALLEL_INSTALL" == "yes" ] && wait "$WAITPROC_PROTOBUF" >/dev/null 2>&1

    echo -e "Setting up ${cc}protobuf$nn"

    mkdir -p protobuf/cmake/build
    cd protobuf/cmake/build
    cmake .. -DCMAKE_C_FLAGS="-fPIC" -DCMAKE_CXX_FLAGS="-Wno-cpp -fPIC" -DCMAKE_C_COMPILER="${T4P4S_CC}" -DCMAKE_CXX_COMPILER="${T4P4S_CXX}" -GNinja  -DBUILD_TESTS=OFF -DBUILD_CONFORMANCE=OFF -DBUILD_EXAMPLES=OFF >$(logfile "protobuf" ".ninja") 2>&1
    sudo ninja install -j ${MAX_MAKE_JOBS} >>$(logfile "protobuf" ".ninja.install") 2>&1
    sudo ldconfig
    cd "$WORKDIR"
fi

if [ "$INSTALL_STAGE4_P4C" == "yes" ]; then
    [ "$PARALLEL_INSTALL" == "yes" ] && wait "$WAITPROC_P4C" >/dev/null 2>&1

    echo -e "Setting up ${cc}p4c$nn"

    export P4C=`pwd`/p4c

    mkdir p4c/build
    cd p4c/build
    cmake .. -DCMAKE_C_FLAGS="-fPIC" -DCMAKE_CXX_FLAGS="-Wno-cpp -fPIC" -DCMAKE_C_COMPILER="gcc" -DCMAKE_CXX_COMPILER="g++" -GNinja  -DENABLE_P4TEST=ON -DENABLE_BMV2=OFF -DENABLE_EBPF=OFF -DENABLE_P4C_GRAPHS=OFF -DENABLE_GTESTS=OFF >$(logfile "p4c" ".ninja") 2>&1
    sudo ninja install -j ${MAX_MAKE_JOBS_P4C} >>$(logfile "p4c" ".ninja.install") 2>&1
    cd "$WORKDIR"
fi


if [ "$INSTALL_STAGE5_GRPC" == "yes" ]; then
    [ "$PARALLEL_INSTALL" == "yes" ] && wait "$WAITPROC_GRPC" >/dev/null 2>&1
    echo -e "Setting up ${cc}grpc$nn"
    mkdir -p grpc/build
    cd grpc/build
    cmake .. -GNinja >$(logfile "grpc" ".cmake") 2>&1
    [ $? -ne 0 ] && echo -e "Error during the compilation of ${cc}grpc$nn (code $cc$?$nn)"
    ninja >>$(logfile "grpc" ".ninja") 2>&1
    [ $? -ne 0 ] && echo -e "Error during the compilation of ${cc}grpc$nn (code $cc$?$nn)"
    sudo ninja install >>$(logfile "grpc" ".ninja.install") 2>&1
    [ $? -ne 0 ] && echo -e "Error during the compilation of ${cc}grpc$nn (code $cc$?$nn)"
    cd "$WORKDIR"

    [ "$PARALLEL_INSTALL" == "yes" ] && wait "$WAITPROC_PI" >/dev/null 2>&1
    echo -e "Setting up ${cc}PI$nn"
    cd PI
    ./autogen.sh >$(logfile "PI" ".autogen") 2>&1
    [ $? -ne 0 ] && echo -e "Error during the compilation of ${cc}PI$nn (code $cc$?$nn)"
    CC=${T4P4S_CC} CC_LD=${T4P4S_LD} CXX="${T4P4S_CXX}" ./configure --with-proto >>$(logfile "PI" ".configure") 2>&1
    [ $? -ne 0 ] && echo -e "Error during the compilation of ${cc}PI$nn (code $cc$?$nn)"
    make -j $MAX_MAKE_JOBS >>$(logfile "PI" ".make") 2>&1
    [ $? -ne 0 ] && echo -e "Error during the compilation of ${cc}PI$nn (code $cc$?$nn)"
    sudo make install -j $MAX_MAKE_JOBS >>$(logfile "PI" ".make.install") 2>&1
    [ $? -ne 0 ] && echo -e "Error during the compilation of ${cc}PI$nn (code $cc$?$nn)"
    cd "$WORKDIR"

    [ "$PARALLEL_INSTALL" == "yes" ] && wait "$WAITPROC_P4Runtime_GRPCPP" >/dev/null 2>&1
    echo -e "Setting up ${cc}P4Runtime_GRPCPP$nn"
    cd P4Runtime_GRPCPP
    ./install.sh >$(logfile "P4Runtime_GRPCPP" ".install") 2>&1
    CC=${T4P4S_CC} CC_LD=${T4P4S_LD} CXX="${T4P4S_CXX}" ./compile.sh >>$(logfile "P4Runtime_GRPCPP" ".compile") 2>&1
    cd "$WORKDIR"
fi



# Save environment config
if [ "$INSTALL_STAGE6_T4P4S" == "yes" ]; then
    [ "$PARALLEL_INSTALL" == "yes" ] && wait "$WAITPROC_T4P4S" >/dev/null 2>&1

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
