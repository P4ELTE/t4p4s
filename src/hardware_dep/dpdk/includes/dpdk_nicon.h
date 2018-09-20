// Copyright 2018 Eotvos Lorand University, Budapest, Hungary
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __WITH_NIC_H_
#define __WITH_NIC_H_


#include "dpdk_lib.h"
#include <stdbool.h>

#define T4P4S_BROADCAST_PORT    100

#define MAX_PKT_BURST     32
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
};


#endif
