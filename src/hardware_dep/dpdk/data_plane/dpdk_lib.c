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
#include "dpdk_lib.h"

#include <rte_ethdev.h>
#include <rte_ip.h>
#include <unistd.h>


extern int numa_on;

extern void sleep_millis(int millis);
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

struct ether_addr ports_eth_addr[RTE_MAX_ETHPORTS];

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
