/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2015 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
// A large portion of the code in this file comes from
// main.c in the l3fwd example of DPDK 2.2.0.

#include "dpdk_lib.h"
#include <rte_ethdev.h>

#include "gen_include.h"


// TODO from...
extern void initialize_args(int argc, char **argv);
extern void initialize_nic();
extern int init_lcore_confs();
extern int init_tables();
extern int init_memories();

// TODO from...
extern void init_control_plane();

// defined in the generated file dataplane.c
extern void handle_packet(packet_descriptor_t* pd, lookup_table_t** tables);

// defined separately for each example
extern bool core_is_working(struct lcore_data* lcdata);
extern bool receive_packet(packet_descriptor_t* pd, struct lcore_data* lcdata, unsigned pkt_idx);
extern void free_packet(packet_descriptor_t* pd);
extern bool is_packet_handled(packet_descriptor_t* pd, struct lcore_data* lcdata);
extern void init_service();
extern void main_loop_pre_rx(struct lcore_data* lcdata);
extern void main_loop_post_rx(struct lcore_data* lcdata);
extern void main_loop_post_single_rx(struct lcore_data* lcdata, bool got_packet);
extern unsigned get_portid(struct lcore_data* lcdata, unsigned queue_idx);
extern void main_loop_rx_group(struct lcore_data* lcdata, unsigned queue_idx);
extern unsigned get_pkt_count_in_group(struct lcore_data* lcdata);
extern unsigned get_queue_count(struct lcore_data* lcdata);
extern void send_packet(packet_descriptor_t* pd, int egress_port, int ingress_port);
extern struct lcore_data init_lcore_data();

//=============================================================================

static void set_metadata_inport(packet_descriptor_t* pd, uint32_t inport)
{
    //modify_field_to_const(pd, field_desc(field_instance_standard_metadata_ingress_port), (uint8_t*)&inport, 2);
    int res32; // needed for the macro
    MODIFY_INT32_INT32_BITS_PACKET(pd, header_instance_standard_metadata, field_standard_metadata_t_ingress_port, inport);
    //MODIFY_INT32_INT32_BITS(pd, field_instance_standard_metadata_ingress_port, inport); // TODO fix? LAKI
}


void do_single_tx(packet_descriptor_t* pd, unsigned queue_idx, unsigned pkt_idx)
{
    if (unlikely(pd->dropped)) {
        debug("  :::: DROPPING\n");
        free_packet(pd);
    } else {
        debug("  :::: EGRESSING\n");

        int egress_port = EXTRACT_EGRESSPORT(pd);
        int ingress_port = EXTRACT_INGRESSPORT(pd);

        send_packet(pd, egress_port, ingress_port);
    }
}

void do_single_rx(struct lcore_data* lcdata, packet_descriptor_t* pd, unsigned queue_idx, unsigned pkt_idx)
{
    bool got_packet = receive_packet(pd, lcdata, pkt_idx);

    if (got_packet) {
	    set_metadata_inport(pd, get_portid(lcdata, queue_idx));
	    if (likely(is_packet_handled(pd, lcdata))) {
	        handle_packet(pd, lcdata->conf->state.tables);
            do_single_tx(pd, queue_idx, pkt_idx);
        }
    }

    main_loop_post_single_rx(lcdata, got_packet);
}

void do_rx(struct lcore_data* lcdata, packet_descriptor_t* pd)
{
    unsigned queue_count = get_queue_count(lcdata);
    for (unsigned queue_idx = 0; queue_idx < queue_count; queue_idx++) {
        main_loop_rx_group(lcdata, queue_idx);

        unsigned pkt_count = get_pkt_count_in_group(lcdata);
        for (unsigned pkt_idx = 0; pkt_idx < pkt_count; pkt_idx++) {
            do_single_rx(lcdata, pd, queue_idx, pkt_idx);
        }
    }
}

void dpdk_main_loop()
{
    struct lcore_data lcdata = init_lcore_data();
    if (!lcdata.is_valid) {
    	debug("lcore data is invalid, exiting\n");
    	return;
    }

    packet_descriptor_t pd;
    init_dataplane(&pd, lcdata.conf->state.tables);

    while (core_is_working(&lcdata)) {
        main_loop_pre_rx(&lcdata);

        do_rx(&lcdata, &pd);

        main_loop_post_rx(&lcdata);
    }
}


static int
launch_one_lcore(__attribute__((unused)) void *dummy)
{
    dpdk_main_loop();
    return 0;
}

int launch_dpdk()
{
    init_service();

    rte_eal_mp_remote_launch(launch_one_lcore, NULL, CALL_MASTER);

    unsigned lcore_id;
    RTE_LCORE_FOREACH_SLAVE(lcore_id) {
        if (rte_eal_wait_lcore(lcore_id) < 0)
            return -1;
    }
    return 0;
}

int main(int argc, char** argv)
{
    initialize_args(argc, argv);
    initialize_nic();
    init_tables();
    init_memories();
    init_lcore_confs();
    init_control_plane();

    int retval = launch_dpdk();

    debug("Exiting program.\n");
    return retval;
}
