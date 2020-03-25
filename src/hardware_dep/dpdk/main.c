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
#include <rte_mempool.h>

#include "gen_include.h"

#ifndef T4P4S_NIC_VARIANT
#error The NIC variant is undefined
#endif

#ifdef T4P4S_SUPPRESS_EAL
    #include <unistd.h>
    #include <stdio.h>
#endif

// TODO from...
extern void initialize_args(int argc, char **argv);
extern void initialize_nic();
extern int init_tables();
extern int init_memories();

extern int flush_tables();

extern int launch_count();
extern void t4p4s_abnormal_exit(int retval, int idx);
extern void t4p4s_pre_launch(int idx);
extern void t4p4s_post_launch(int idx);
extern void t4p4s_normal_exit();

// defined in the generated file controlplane.c
extern void init_control_plane();

// defined in the generated file dataplane.c
extern void handle_packet(packet_descriptor_t* pd, lookup_table_t** tables, parser_state_t* pstate, uint32_t portid);

// defined separately for each example
extern bool core_is_working(struct lcore_data* lcdata);
extern bool fetch_packet(packet_descriptor_t* pd, struct lcore_data* lcdata, unsigned pkt_idx);
extern void free_packet(packet_descriptor_t* pd);
extern bool is_packet_handled(packet_descriptor_t* pd, struct lcore_data* lcdata);
extern void init_storage();
extern void main_loop_pre_rx(struct lcore_data* lcdata);
extern void main_loop_post_rx(struct lcore_data* lcdata);
extern void main_loop_post_single_rx(struct lcore_data* lcdata, bool got_packet);
extern uint32_t get_portid(struct lcore_data* lcdata, unsigned queue_idx);
extern unsigned do_rx_burst(struct lcore_data* lcdata, unsigned queue_idx);
extern unsigned get_queue_count(struct lcore_data* lcdata);
extern void send_single_packet(struct lcore_data* lcdata, packet_descriptor_t* pd, packet* pkt, int egress_port, int ingress_port);
extern void send_broadcast_packet(struct lcore_data* lcdata, packet_descriptor_t* pd, int egress_port, int ingress_port);
extern struct lcore_data init_lcore_data();
extern packet* clone_packet(packet* pd, struct rte_mempool* mempool);

// -----------------------------------------------------------------------------

#include "dpdkx_crypto.c"

//=============================================================================

extern uint32_t get_port_mask();
extern uint8_t get_port_count();

void broadcast_packet(struct lcore_data* lcdata, packet_descriptor_t* pd, int egress_port, int ingress_port)
{
    uint8_t nb_ports = get_port_count();
    uint32_t port_mask = get_port_mask();
    debug("    : Mask port " T4LIT(%d,port) "\n", nb_ports);
    debug("    : Mask port " T4LIT(%x,port) "\n", port_mask);

    uint8_t nb_port = 0;
    for (uint8_t portidx = 0; nb_port < nb_ports - 1 && portidx < RTE_MAX_ETHPORTS; ++portidx) {
        if (portidx == ingress_port) {
            debug("    : Skipping broadcast on ingress port " T4LIT(%d,port), ingress_port);
           continue;
        }

        bool is_port_disabled = (port_mask & (1 << portidx)) == 0;
        if (is_port_disabled)   continue;

        debug("    : Broadcasting on port " T4LIT(%d,port) "\n", portidx);

        packet* pkt_out = (nb_port < nb_ports) ? clone_packet(pd->wrapper, lcdata->conf->mempool) : pd->wrapper;
        send_single_packet(lcdata, pd, pkt_out, egress_port, ingress_port);

        nb_port++;
    }

    if (unlikely(nb_port != nb_ports - 1)) {
        debug(T4LIT(Wrong port count,error) ": " T4LIT(%d) " ports should be present, but only " T4LIT(%d) " found", nb_ports, nb_port);
    }
}

/* Enqueue a single packet, and send burst if queue is filled */
void send_packet(struct lcore_data* lcdata, packet_descriptor_t* pd, int egress_port, int ingress_port)
{
    uint32_t lcore_id = rte_lcore_id();
    struct rte_mbuf* mbuf = (struct rte_mbuf *)pd->wrapper;

    if (unlikely(egress_port == T4P4S_BROADCAST_PORT)) {
        dbg_bytes(rte_pktmbuf_mtod(mbuf, uint8_t*), rte_pktmbuf_pkt_len(mbuf), "   :: Broadcasting packet from port " T4LIT(%d,port) " (" T4LIT(%d) " bytes): ", ingress_port, rte_pktmbuf_pkt_len(mbuf));
        broadcast_packet(lcdata, pd, egress_port, ingress_port);
    } else {
        dbg_bytes(rte_pktmbuf_mtod(mbuf, uint8_t*), rte_pktmbuf_pkt_len(mbuf), "   :: Emitting packet on port " T4LIT(%d,port) " (" T4LIT(%d) " bytes): ", egress_port, rte_pktmbuf_pkt_len(mbuf));
        send_single_packet(lcdata, pd, pd->wrapper, egress_port, ingress_port);
    }
}

void do_single_tx(struct lcore_data* lcdata, packet_descriptor_t* pd)
{
    if (unlikely(pd->dropped)) {
        debug(" :::: Dropping packet\n");
        free_packet(pd);
    } else {
        debug(" :::: Egressing packet\n");

        int egress_port = EXTRACT_EGRESSPORT(pd);
        int ingress_port = EXTRACT_INGRESSPORT(pd);

        send_packet(lcdata, pd, egress_port, ingress_port);
    }
}

void do_handle_packet(struct lcore_data* lcdata, packet_descriptor_t* pd, uint32_t port_id)
{
    debug_mbuf(pd->wrapper,"Arrived message to do_handle_packet");
    handle_packet(pd, lcdata->conf->state.tables, &(lcdata->conf->state.parser_state), port_id);

    do_single_tx(lcdata, pd);
    #if ASYNC_MODE == ASYNC_MODE_CONTEXT
        if(pd->context != NULL)
        {
            debug(T4LIT(Context for packet %p terminating... swapping back to main context...,warning) "\n", pd->context);
            rte_ring_enqueue(context_buffer, pd->context);
        }
    #endif
}

// defined in main_async.c
void async_handle_packet(struct lcore_data* lcdata, packet_descriptor_t* pd, unsigned pkt_idx, uint32_t port_id, void (*handler_function)(void));
void main_loop_async(struct lcore_data* lcdata, packet_descriptor_t* pd);
void main_loop_fake_crypto(struct lcore_data* lcdata);

#ifdef DEBUG__CONTEXT_SWITCH_FOR_EVERY_N_PACKET
    int packet_required_counter[RTE_MAX_LCORE];
#endif
void do_single_rx(struct lcore_data* lcdata, packet_descriptor_t* pd, unsigned queue_idx, unsigned pkt_idx)
{
    pd->context = NULL;
    pd->dropped = 0;
    bool got_packet = fetch_packet(pd, lcdata, pkt_idx);

    if (got_packet) {
        void (*handler_function)(struct lcore_data* lcdata, packet_descriptor_t* pd, uint32_t port_id) = do_handle_packet;
        if (likely(is_packet_handled(pd, lcdata))) {
            pd->port_id = get_portid(lcdata, queue_idx);
        #if ASYNC_MODE == ASYNC_MODE_CONTEXT
            if(PACKET_REQUIRES_ASYNC(pd)){
                async_handle_packet(lcdata, pd, pkt_idx, get_portid(lcdata, queue_idx), (void (*)(void))handler_function);
            }
            else
        #endif
                handler_function(lcdata, pd, get_portid(lcdata, queue_idx));
        }
    }
    main_loop_post_single_rx(lcdata, got_packet);
}

void do_rx(struct lcore_data* lcdata, packet_descriptor_t* pd)
{
    unsigned queue_count = get_queue_count(lcdata);
    for (unsigned queue_idx = 0; queue_idx < queue_count; queue_idx++) {
        unsigned pkt_count = do_rx_burst(lcdata, queue_idx);
        for (unsigned pkt_idx = 0; pkt_idx < pkt_count; pkt_idx++) {
            do_single_rx(lcdata, pd, queue_idx, pkt_idx);
        }
    }
}

void main_loop_whole_process(struct lcore_data* lcdata, packet_descriptor_t* pd){
    main_loop_pre_rx(lcdata);

    do_rx(lcdata, pd);
    main_loop_post_rx(lcdata);
}

bool dpdk_main_loop()
{
    extern struct lcore_conf lcore_conf[RTE_MAX_LCORE];
    uint32_t lcore_id = rte_lcore_id();

    struct lcore_data lcdata = init_lcore_data();
    #ifdef START_CRYPTO_NODE
        if (!lcdata.is_valid && rte_lcore_id() != rte_lcore_count() - 1) {
            debug("lcore data is invalid, exiting\n");
            return false;
        }
        if (lcore_id ==  rte_lcore_count() - 1){
            RTE_LOG(INFO, P4_FWD, "lcore %u is the crypto node\n", lcore_id);
        }
    #else
        if (!lcdata.is_valid) {
            debug("lcore data is invalid, exiting\n");
            return false;
        }
    #endif

    packet_descriptor_t pd;
    init_dataplane(&pd, lcdata.conf->state.tables);

    #if ASYNC_MODE == ASYNC_MODE_CONTEXT
        getcontext(&lcore_conf[rte_lcore_id()].main_loop_context);
    #endif

    while (core_is_working(&lcdata)) {
        #ifdef START_CRYPTO_NODE
            if (lcore_id ==  rte_lcore_count() - 1){
                main_loop_fake_crypto(&lcdata);
            }else{
                main_loop_whole_process(&lcdata,&pd);
            }
        #else
            main_loop_whole_process(&lcdata,&pd);
        #endif

        #if ASYNC_MODE != ASYNC_MODE_OFF
            main_loop_async(&lcdata, &pd);
        #endif
    }

    return lcdata.is_valid;
}


static int
launch_one_lcore(__attribute__((unused)) void *dummy)
{
    bool success = dpdk_main_loop();
    return success ? 0 : -1;
}

int launch_dpdk()
{
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
    RTE_LOG(INFO, P4_FWD, ":: Starter config :: \n");
    RTE_LOG(INFO, P4_FWD, " -- ASYNC_MODE: %u\n", ASYNC_MODE);
    RTE_LOG(INFO, P4_FWD, " -- CRYPTO_NODE_MODE: %u\n", CRYPTO_NODE_MODE);
    #ifdef  DEBUG__CRYPTO_EVERY_N
        RTE_LOG(INFO, P4_FWD, " -- DEBUG__CRYPTO_EVERY_N: %u\n", DEBUG__CRYPTO_EVERY_N);
    #endif

    RTE_LOG(INFO, P4_FWD, " -- FAKE_CRYPTO_SLEEP_MULTIPLIER: %u\n", FAKE_CRYPTO_SLEEP_MULTIPLIER);

    debug("Initializing switch\n");

    initialize_args(argc, argv);
    initialize_nic();

    int launch_count2 = launch_count();
    for (int i = 0; i < launch_count2; ++i) {
        debug("Initializing execution\n");

        init_tables();
        init_storage();

        init_memories();
        debug("   :: Initializing control plane connection\n");
        init_control_plane();
        debug("   :: Initializing storage\n");

        t4p4s_pre_launch(i);

        int retval = launch_dpdk();
        if (retval < 0) {
            t4p4s_abnormal_exit(retval, i);
            return retval;
        }

        t4p4s_post_launch(i);

        flush_tables();
    }

    t4p4s_normal_exit();
    return 0;
}
