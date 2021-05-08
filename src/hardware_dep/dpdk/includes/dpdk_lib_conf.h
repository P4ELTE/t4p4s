// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include <rte_version.h>

#include "dataplane_lookup.h"  // lookup_table_t
#include "parser.h"     // parser_state_t
#include "common.h"     // NB_TABLES
#include "util_debug.h"

#include "dpdk_lib_conf_async.h"

//=============================================================================
// Unifying renamed types and constants

#if RTE_VERSION >= RTE_VERSION_NUM(19,8,0,0)
    typedef struct rte_ether_addr rte_eth_addr_t;
    #define RTE_MAX_ETHPORT_COUNT RTE_MAX_ETHPORTS
    #define RTE_ETH_MAX_LEN RTE_ETHER_MAX_LEN
#else
    typedef struct ether_addr rte_eth_addr_t;
    #define RTE_MAX_ETHPORT_COUNT MAX_ETHPORTS
    #define RTE_ETH_MAX_LEN ETHER_MAX_LEN
#endif

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

#include <ucontext.h>
#include <signal.h>

// SIGSTKSZ is typically 8192
#ifdef T4P4S_DEBUG
#define CONTEXT_STACKSIZE SIGSTKSZ*2
#else
#define CONTEXT_STACKSIZE SIGSTKSZ
#endif
#include <setjmp.h>

struct lcore_conf {
    struct lcore_hardware_conf hw;
    struct lcore_state         state;
    struct rte_mempool*        mempool;
    struct rte_mempool*        crypto_pool;
    ucontext_t                 main_loop_context;
    struct rte_ring*           async_queue;
    unsigned                   pending_crypto;
    struct rte_ring    *fake_crypto_rx;
    struct rte_ring    *fake_crypto_tx;

    occurence_counter_t async_drop_counter;
    occurence_counter_t fwd_packet;
    occurence_counter_t sent_to_crypto_packet;
    occurence_counter_t doing_crypto_packet;
    occurence_counter_t async_packet;
	occurence_counter_t processed_packet_num;

    jmp_buf mainLoopJumpPoint;
    jmp_buf asyncLoopJumpPoint;

    #ifdef DEBUG__CRYPTO_EVERY_N
        int crypto_every_n_counter;
    #endif
} __rte_cache_aligned;
