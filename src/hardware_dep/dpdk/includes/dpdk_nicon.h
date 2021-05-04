// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#pragma once

#define T4P4S_NIC_VARIANT on

#include "aliases.h"
#include "dpdk_lib_conf.h"
#include <stdbool.h>

#include "testsuite.h"

#define T4P4S_BROADCAST_PORT    100

#define MAX_PKT_BURST     32  /* note: this equals to MBUF_TABLE_SIZE in dpdk_lib.h */
#define BURST_TX_DRAIN_US 100 /* TX drain every ~100us */

#define MAX_PORTS               16

#define MCAST_CLONE_PORTS       2
#define MCAST_CLONE_SEGS        2

#define NB_PKT_MBUF             8192

#define HDR_MBUF_DATA_SIZE      (2 * RTE_PKTMBUF_HEADROOM)
#define NB_HDR_MBUF             (NB_PKT_MBUF * MAX_PORTS)

#define NB_CLONE_MBUF           (NB_PKT_MBUF * MCAST_CLONE_PORTS * MCAST_CLONE_SEGS * 2)

// note: this much space MUST be able to hold all deparsed content
#define DEPARSE_BUFFER_SIZE     1024


struct lcore_data {
    const uint64_t      drain_tsc;
    uint64_t            prev_tsc;

    struct lcore_conf*  conf;

    packet*             pkts_burst[MAX_PKT_BURST];
    unsigned            nb_rx;

    bool                is_valid;

    struct rte_mempool* mempool;
};
