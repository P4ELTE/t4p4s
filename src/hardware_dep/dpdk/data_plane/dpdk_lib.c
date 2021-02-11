// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_lib.h"
#include "util_debug.h"

#include <rte_ethdev.h>
#include <rte_ip.h>
#include <unistd.h>


extern int numa_on;

extern void print_all_ports_link_status(uint8_t port_num, uint32_t port_mask);
extern void print_port_mac(unsigned portid, uint8_t* mac_bytes);
extern void table_set_default_action(lookup_table_t* t, uint8_t* value);

//=============================================================================
// Shared

struct socket_state state[NB_SOCKETS];
struct lcore_conf lcore_conf[RTE_MAX_LCORE];

uint32_t enabled_port_mask;

uint16_t            nb_lcore_params;
struct lcore_params lcore_params[MAX_LCORE_PARAMS];

//=============================================================================
// Locals

struct rte_mempool* pktmbuf_pool[NB_SOCKETS];

rte_eth_addr_t ports_eth_addr[RTE_MAX_ETHPORT_COUNT];

//=============================================================================
// Getters

int get_socketid(unsigned lcore_id)
{
    int socketid = numa_on ? rte_lcore_to_socket_id(lcore_id) : 0;

    if (unlikely(socketid >= NB_SOCKETS)) {
        rte_exit(EXIT_FAILURE, "Socket %d of lcore %u is out of range %d\n",
                 socketid, lcore_id, NB_SOCKETS);
    }

    return socketid;
}

// Returns the number of rx queues that this port has.
uint8_t get_port_n_rx_queues(uint8_t port)
{
    int queue = -1;

    for (uint16_t i = 0; i < nb_lcore_params; ++i) {
        if (lcore_params[i].port_id == port && lcore_params[i].queue_id > queue)
            queue = lcore_params[i].queue_id;
    }
    return (uint8_t)(++queue);
}

//=============================================================================
// Direct includes

#include "dpdk_lib_change_tables.c"
#include "dpdk_lib_init_tables.c"
#include "dpdk_lib_init_hw.c"
#include "dpdk_lib_parse_args.c"
#include "dpdk_lib_print.c"

//=============================================================================
// Calculations

uint16_t calculate_csum16(const void* buf, uint16_t length) {
    uint16_t value16 = rte_raw_cksum(buf, length);
    return value16;
}

uint32_t packet_length(packet_descriptor_t* pd) {
    return rte_pktmbuf_pkt_len(pd->wrapper);
}


packet* clone_packet(packet* pd, struct rte_mempool* mempool) {
    return rte_pktmbuf_clone(pd, mempool);
}
