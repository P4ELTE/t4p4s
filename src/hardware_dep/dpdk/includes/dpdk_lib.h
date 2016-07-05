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

#include <rte_common.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_memzone.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ring.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_vect.h>
#include <rte_byteorder.h>
#include <rte_string_fns.h>

#include <getopt.h>

//=============================================================================

#include "data_plane_data.h"
#include "backend.h"
#include "dataplane.h" // lookup_table_t
#include "dpdk_tables.h"
#include "ctrl_plane_backend.h"

//=============================================================================
// Backend-specific aliases

#include "aliases.h"

#define parse_as rte_pktmbuf_mtod
#define MAX_ETHPORTS RTE_MAX_ETHPORTS

//=============================================================================
// Shared types and constants

extern uint32_t enabled_port_mask;
extern int promiscuous_on;
extern int numa_on;

#define RTE_LOGTYPE_L3FWD RTE_LOGTYPE_USER1 // rte_log.h
#define RTE_LOGTYPE_L2FWD RTE_LOGTYPE_USER1 // rte_log.h
#define RTE_LOGTYPE_P4_FWD RTE_LOGTYPE_USER1 // rte_log.h

#define MBUF_SIZE (2048 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)
// TODO is the longer version better?
/*
#define NB_MBUF RTE_MAX	(																	\
				(nb_ports*nb_rx_queue*RTE_TEST_RX_DESC_DEFAULT +							\
				nb_ports*nb_lcores*MAX_PKT_BURST +											\
				nb_ports*n_tx_queue*RTE_TEST_TX_DESC_DEFAULT +								\
				nb_lcores*MEMPOOL_CACHE_SIZE),												\
				(unsigned)8192)
*/
#define NB_MBUF 8192
#define MEMPOOL_CACHE_SIZE 256

#define MAX_PKT_BURST     32
#define BURST_TX_DRAIN_US 100 /* TX drain every ~100us */

struct mbuf_table {
	uint16_t len;
	struct rte_mbuf *m_table[MAX_PKT_BURST];
};

/* there was 1 1 1 */
#define MAX_RX_QUEUE_PER_LCORE 16
#define MAX_TX_QUEUE_PER_PORT RTE_MAX_ETHPORTS
#define MAX_RX_QUEUE_PER_PORT 128

#define NB_SOCKETS 8
//#define	BAD_PORT	((uint16_t)-1)

#ifndef NB_TABLES
#define NB_TABLES 0
#endif

struct lcore_rx_queue {
	uint8_t port_id;
	uint8_t queue_id;
} __rte_cache_aligned;

#define NB_REPLICA 2

struct lcore_state {
    // pointers to the containing socket's instance
    lookup_table_t * tables   [NB_TABLES];
    counter_t      * counters [NB_COUNTERS];
};

struct socket_state {
    // pointers to the instances created on each socket
    lookup_table_t * tables         [NB_TABLES][NB_REPLICA];
    int              active_replica [NB_TABLES];
    counter_t      * counters       [NB_COUNTERS][RTE_MAX_LCORE];
};

struct lcore_conf {
        // message queues
        uint64_t tx_tsc;
        uint16_t n_rx_queue;
        struct lcore_rx_queue rx_queue_list[MAX_RX_QUEUE_PER_LCORE];
        uint16_t tx_queue_id[RTE_MAX_ETHPORTS];
        struct mbuf_table tx_mbufs[RTE_MAX_ETHPORTS];
        // pointers to "stateful memories"
        struct lcore_state state;
} __rte_cache_aligned;

#define TABCHANGE_DELAY 50 // microseconds

/* Per-port statistics struct */
struct l2fwd_port_statistics {
	uint64_t tx;
	uint64_t rx;
	uint64_t dropped;
} __rte_cache_aligned;

extern struct l2fwd_port_statistics port_statistics[RTE_MAX_ETHPORTS];

#define PRINT_OPAQUE_STRUCT(p)  print_mem((p), sizeof(*(p)))
static void print_mem_hex(void const *vp, size_t n) {
    unsigned char const *p = vp;
    for (size_t i=0; i<n; i++) printf("%02x ", p[i]);
    putchar('\n');
};
static void print_mem_bin(void const * const ptr, size_t const size)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;
//    for (i=size-1;i>=0;i--) for (j=7;j>=0;j--) {
    for (i=0;i<size;i++) for (j=0;j<8;j++) {
        byte = b[i] & (1<<j);
        byte >>= j;
        printf("%u", byte);
    }
    puts("");
}

#endif // DPDK_LIB_H
