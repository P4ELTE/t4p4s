// Copyright 2016 Eotvos Lorand University, Budapest, Hungary
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
#ifndef DPDK_LIB_H
#define DPDK_LIB_H

//=============================================================================

#include <rte_atomic.h>
#include <rte_byteorder.h>
#include <rte_common.h>
#include <rte_cycles.h>
#include <rte_debug.h>
#include <rte_eal.h>
#include <rte_interrupts.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_memcpy.h>
#include <rte_memory.h>
#include <rte_mempool.h>
#include <rte_memzone.h>
#include <rte_pci.h>
#include <rte_per_lcore.h>
#include <rte_prefetch.h>
#include <rte_random.h>
#include <rte_ring.h>
#include <rte_string_fns.h>
#include <rte_vect.h>

#include <getopt.h>

//=============================================================================

#include "backend.h"
#include "dataplane.h" // lookup_table_t
#include "parser.h" // parser_state_t
#include "dpdk_tables.h"
#include "tables.h"
#include "ctrl_plane_backend.h"

//=============================================================================
// Backend-specific aliases

#include "aliases.h"
#include "stateful_memory.h"

#define MAX_ETHPORTS RTE_MAX_ETHPORTS

//=============================================================================
// Shared types and constants

#define RTE_LOGTYPE_L3FWD RTE_LOGTYPE_USER1 // rte_log.h
#define RTE_LOGTYPE_L2FWD RTE_LOGTYPE_USER1 // rte_log.h
#define RTE_LOGTYPE_P4_FWD RTE_LOGTYPE_USER1 // rte_log.h

#define MBUF_SIZE (2048 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)

#define NB_MBUF 8192
#define MEMPOOL_CACHE_SIZE 256

#define MAX_JUMBO_PKT_LEN  9600

#define MBUF_TABLE_SIZE 32

struct mbuf_table {
	uint16_t len;
	struct rte_mbuf *m_table[MBUF_TABLE_SIZE];
};

#define RTE_TEST_RX_DESC_DEFAULT 128
#define RTE_TEST_TX_DESC_DEFAULT 512

#define MAX_LCORE_PARAMS 1024

#define MAX_RX_QUEUE_PER_LCORE 16
#define MAX_TX_QUEUE_PER_PORT RTE_MAX_ETHPORTS
#define MAX_RX_QUEUE_PER_PORT 128

#define NB_SOCKETS 8

struct lcore_rx_queue {
	uint8_t port_id;
	uint8_t queue_id;
} __rte_cache_aligned;

#define NB_REPLICA 2

struct lcore_params {
    uint8_t port_id;
    uint8_t queue_id;
    uint8_t lcore_id;
} __rte_cache_aligned;

struct lcore_state {
    lookup_table_t* tables[NB_TABLES];
    parser_state_t parser_state;
};

struct socket_state {
    // pointers to the instances created on each socket
    lookup_table_t * tables         [NB_TABLES][NB_REPLICA];
    int              active_replica [NB_TABLES];
};

struct lcore_hardware_conf {
    // message queues
    uint64_t tx_tsc;
    uint16_t n_rx_queue;
    struct lcore_rx_queue rx_queue_list[MAX_RX_QUEUE_PER_LCORE];
    uint16_t tx_queue_id[RTE_MAX_ETHPORTS];
    struct mbuf_table tx_mbufs[RTE_MAX_ETHPORTS];
};

struct lcore_conf {
    struct lcore_hardware_conf hw;
    struct lcore_state         state;
} __rte_cache_aligned;


//=============================================================================
// Timings

#define TABCHANGE_SLEEP_MICROS 200
#define DIGEST_SLEEP_MILLIS    1000


#endif // DPDK_LIB_H
