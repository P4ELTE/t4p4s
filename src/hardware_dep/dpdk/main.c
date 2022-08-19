// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Eotvos Lorand University, Budapest, Hungary

#include "gen_include.h"
#include "main.h"

#include <rte_ethdev.h>
#include <rte_mempool.h>


volatile int packet_counter = 0;
volatile int packet_with_error_counter = 0;


void get_broadcast_port_msg(char result[256], int ingress_port) {
    uint8_t nb_ports = get_port_count();
    uint32_t port_mask = get_port_mask();

    char* result_ptr = result;
    bool is_first_printed_port = true;
    for (uint8_t portidx = 0; portidx < RTE_MAX_ETHPORTS; ++portidx) {
        if (portidx == ingress_port) {
           continue;
        }

        bool is_port_disabled = (port_mask & (1 << portidx)) == 0;
        if (is_port_disabled)   continue;

        int printed_bytes = sprintf(result_ptr, "%s" T4LIT(%d,port), is_first_printed_port ? "" : ", ", portidx);
        result_ptr += printed_bytes;
        is_first_printed_port = false;
    }
}


void broadcast_packet(int egress_port, int ingress_port, LCPARAMS)
{
    uint8_t nb_ports = get_port_count();
    uint32_t port_mask = get_port_mask();

    uint8_t nb_port = 0;
    for (uint8_t portidx = 0; nb_port < nb_ports - 1 && portidx < RTE_MAX_ETHPORTS; ++portidx) {
        if (portidx == ingress_port) {
           continue;
        }

        bool is_port_disabled = (port_mask & (1 << portidx)) == 0;
        if (is_port_disabled)   continue;

        packet* pkt_out = (nb_port < nb_ports-2) ? clone_packet(pd->wrapper, lcdata->mempool) : pd->wrapper;
        send_single_packet(pkt_out, portidx, ingress_port, false, LCPARAMS_IN);

        ++nb_port;
    }

    if (unlikely(nb_port != nb_ports - 1)) {
        debug(" " T4LIT(!!!!,error) " " T4LIT(Wrong port count,error) ": " T4LIT(%d) " ports should be present, but only " T4LIT(%d) " found\n", nb_ports, nb_port);
    }
}

/* Enqueue a single packet, and send burst if queue is filled */
void send_packet(int egress_port, int ingress_port, LCPARAMS)
{
    uint32_t lcore_id = rte_lcore_id();
    struct rte_mbuf* mbuf = (struct rte_mbuf *)pd->wrapper;

    if (unlikely(egress_port == T4P4S_BROADCAST_PORT)) {
        #ifdef T4P4S_DEBUG
            char ports_msg[256];
            get_broadcast_port_msg(ports_msg, ingress_port);
            debug(" " T4LIT(<<<<,outgoing) " " T4LIT(Broadcasting,outgoing) " packet " T4LIT(#%03d) "%s from port " T4LIT(%d,port) " to all other ports (%s): " T4LIT(%dB) " of headers, " T4LIT(%dB) " of payload\n",
                  get_packet_idx(LCPARAMS_IN),  pd->is_deparse_reordering ? "" : " with " T4LIT(unchanged structure),
                  ingress_port, ports_msg, rte_pktmbuf_pkt_len(mbuf) - pd->payload_size, pd->payload_size);
        #endif
        broadcast_packet(egress_port, ingress_port, LCPARAMS_IN);
    } else {
        debug(" " T4LIT(<<<<,outgoing) " " T4LIT(Emitting,outgoing) " packet " T4LIT(#%03d) "%s on port " T4LIT(%d,port) ": " T4LIT(%dB) " of headers, " T4LIT(%dB) " of payload\n",
              get_packet_idx(LCPARAMS_IN),  pd->is_deparse_reordering ? "" : " with " T4LIT(unchanged structure),
              egress_port, rte_pktmbuf_pkt_len(mbuf) - pd->payload_size, pd->payload_size);
        send_single_packet(pd->wrapper, egress_port, ingress_port, false, LCPARAMS_IN);
    }
}

void do_single_tx(LCPARAMS)
{
    main_loop_pre_single_tx(LCPARAMS_IN);
    if (unlikely(is_packet_dropped(pd))) {
        free_packet(LCPARAMS_IN);
    } else {
        int egress_port = get_egress_port(pd);
        int ingress_port = get_ingress_port(pd);

        send_packet(egress_port, ingress_port, LCPARAMS_IN);
    }
    main_loop_post_single_tx(LCPARAMS_IN);
}

void do_handle_packet(unsigned port_id, int pkt_idx, LCPARAMS)
{
    struct lcore_state state = lcdata->conf->state;
    lookup_table_t** tables = state.tables;
    parser_state_t* pstate = &(state.parser_state);
    init_parser_state(&(state.parser_state));

    handle_packet(port_id, pkt_idx, STDPARAMS_IN);
    do_single_tx(LCPARAMS_IN);
    #if ASYNC_MODE == ASYNC_MODE_CONTEXT
        if (pd->context != NULL) {
            debug(" " T4LIT(<<<<,async) " Context for packet " T4LIT(%p,bytes) " terminating, swapping back to " T4LIT(main context,async) "\n", pd->context);
            rte_ring_enqueue(context_free_command_ring, pd->context);
        }
    #endif
}
// TODO move this to stats.h.py
extern void print_async_stats(LCPARAMS);

void do_single_rx(unsigned queue_idx, unsigned pkt_burst_iter, LCPARAMS)
{
    print_async_stats(LCPARAMS_IN);
    #ifdef T4P4S_DEBUG
        pd->is_egress_port_set = false;
    #endif

    main_loop_pre_single_rx(LCPARAMS_IN);

    bool got_packet = receive_packet(pkt_burst_iter, LCPARAMS_IN);
    if (likely(got_packet && is_packet_handled(LCPARAMS_IN))) {
        int packet_idx = get_packet_idx(LCPARAMS_IN);
        unsigned port_id = get_portid(queue_idx, LCPARAMS_IN);
        #if defined ASYNC_MODE && ASYNC_MODE != ASYNC_MODE_OFF
            async_handle_packet(port_id, packet_idx, do_handle_packet, LCPARAMS_IN);
        #else
            do_handle_packet(port_id, packet_idx, LCPARAMS_IN);
        #endif
    }

    main_loop_post_single_rx(got_packet, LCPARAMS_IN);
}

bool do_rx(LCPARAMS)
{
    bool got_packet = false;
    unsigned queue_count = get_queue_count(lcdata);
    for (unsigned queue_idx = 0; queue_idx < queue_count; queue_idx++) {
        main_loop_rx_group(queue_idx, LCPARAMS_IN);
        unsigned pkt_count = get_pkt_count_in_group(lcdata);

        got_packet |= pkt_count > 0;
        for (unsigned pkt_burst_iter = 0; pkt_burst_iter < pkt_count; pkt_burst_iter++) {
            do_single_rx(queue_idx, pkt_burst_iter, LCPARAMS_IN);
        }
    }

    return got_packet;
}

void main_loop_pre_do_post_rx(LCPARAMS){
    main_loop_pre_rx(LCPARAMS_IN);
    bool got_packet = do_rx(LCPARAMS_IN);
    main_loop_post_rx(got_packet, LCPARAMS_IN);
    #if defined ASYNC_MODE && ASYNC_MODE != ASYNC_MODE_OFF
        main_loop_async(LCPARAMS_IN);
    #endif
}

int crypto_node_id() {
    return rte_lcore_count() - 1;
}

bool is_crypto_node() {
    return rte_lcore_id() == crypto_node_id();
}

bool initial_check(LCPARAMS) {
    if (!lcdata->is_valid) {
        debug("lcore data is invalid, exiting\n");
        #ifdef START_CRYPTO_NODE
            if (!is_crypto_node()) return false;
        #else
            return false;
        #endif
    }

    #ifdef START_CRYPTO_NODE
        if (is_crypto_node()){
            RTE_LOG(INFO, P4_FWD, "lcore %u is the crypto node\n", lcore_id);
        }
    #endif

    return true;
}

void init_stats(LCPARAMS)
{
    COUNTER_INIT(lcdata->conf->processed_packet_num);
    COUNTER_INIT(lcdata->conf->async_packet);
    COUNTER_INIT(lcdata->conf->sent_to_crypto_packet);
    COUNTER_INIT(lcdata->conf->doing_crypto_packet);
    COUNTER_INIT(lcdata->conf->fwd_packet);
}

void dpdk_main_loop()
{
    extern struct lcore_conf lcore_conf[RTE_MAX_LCORE];
    uint32_t lcore_id = rte_lcore_id();

    struct lcore_data lcdata_content = init_lcore_data();
    packet_descriptor_t pd_content;

    struct lcore_data* lcdata = &lcdata_content;
    packet_descriptor_t* pd = &pd_content;

    if (!initial_check(LCPARAMS_IN))   return;

    init_dataplane(pd, lcdata->conf->state.tables);

    #if defined ASYNC_MODE && ASYNC_MODE == ASYNC_MODE_CONTEXT
        getcontext(&lcore_conf[rte_lcore_id()].main_loop_context);
    #endif

    init_stats(LCPARAMS_IN);

    while (likely(core_is_working(LCPARAMS_IN))) {
        #ifdef START_CRYPTO_NODE
            if (is_crypto_node()) {
                main_loop_fake_crypto(LCPARAMS_IN);
                continue;
            }
        #endif

        main_loop_pre_do_post_rx(LCPARAMS_IN);
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
    #if RTE_VERSION >= RTE_VERSION_NUM(20,11,0,0)
        rte_eal_mp_remote_launch(launch_one_lcore, NULL, CALL_MAIN);

        unsigned lcore_id;
        RTE_LCORE_FOREACH_WORKER(lcore_id) {
            if (rte_eal_wait_lcore(lcore_id) < 0)
                return -1;
        }
    #else
        rte_eal_mp_remote_launch(launch_one_lcore, NULL, CALL_MASTER);

        unsigned lcore_id;
        RTE_LCORE_FOREACH_SLAVE(lcore_id) {
            if (rte_eal_wait_lcore(lcore_id) < 0)
                return -1;
        }
    #endif

    return 0;
}

void init_async()
{
    #if defined ASYNC_MODE && ASYNC_MODE != ASYNC_MODE_OFF
        debug(":: Starter config :: \n");
        debug(" -- ASYNC_MODE: %u\n", ASYNC_MODE);
        #ifdef  DEBUG__CRYPTO_EVERY_N
            debug(" -- DEBUG__CRYPTO_EVERY_N: %u\n", DEBUG__CRYPTO_EVERY_N);
        #endif
        debug(" -- CRYPTO_NODE_MODE: %u\n", CRYPTO_NODE_MODE);
        debug(" -- FAKE_CRYPTO_SLEEP_MULTIPLIER: %u\n", FAKE_CRYPTO_SLEEP_MULTIPLIER);
        debug(" -- CRYPTO_BURST_SIZE: %u\n", CRYPTO_BURST_SIZE);
        debug(" -- CRYPTO_CONTEXT_POOL_SIZE: %u\n", CRYPTO_CONTEXT_POOL_SIZE);
        debug(" -- CRYPTO_RING_SIZE: %u\n", CRYPTO_RING_SIZE);
    #endif
}

int main(int argc, char** argv)
{
    debug("Init switch\n");
    #ifdef T4P4S_VETH_MODE
        debug("Running in VETH mode!\n");
    #endif

    initialize_args(argc, argv);
    initialize_nic();
    init_async();

    int launch_count2 = launch_count();
    for (int idx = 0; idx < launch_count2; ++idx) {
        debug("Init execution\n");

        init_tables();
        init_storage();

        init_memories();
        #ifndef T4P4S_NO_CONTROL_PLANE
            debug(" " T4LIT(::::,incoming) " Init control plane connection on port " T4LIT(%d,port) "\n", T4P4S_CTL_PORT);
            init_control_plane();
        #else
            debug(" :::: (Control plane inactive)\n");
        #endif

        init_table_default_actions();

        t4p4s_pre_launch(idx);

        int retval = launch_dpdk();
        if (retval < 0) {
            t4p4s_abnormal_exit(retval, idx);
            return retval;
        }

        t4p4s_post_launch(idx);

        flush_tables();
    }

    return t4p4s_normal_exit();
}
