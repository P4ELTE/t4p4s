// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Eotvos Lorand University, Budapest, Hungary

#include "gen_include.h"
#include "main.h"

#include <rte_ethdev.h>


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

        packet* pkt_out = (nb_port < nb_ports) ? clone_packet(pd->wrapper, lcdata->mempool) : pd->wrapper;
        send_single_packet(pkt_out, egress_port, ingress_port, false, LCPARAMS_IN);

        nb_port++;
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
            dbg_bytes(rte_pktmbuf_mtod(mbuf, uint8_t*), rte_pktmbuf_pkt_len(mbuf), "   " T4LIT(<<,outgoing) " " T4LIT(Broadcasting,outgoing) " packet from port " T4LIT(%d,port) " to all other ports (%s) (" T4LIT(%d) " bytes): ", ingress_port, ports_msg, rte_pktmbuf_pkt_len(mbuf));
        #endif
        broadcast_packet(egress_port, ingress_port, LCPARAMS_IN);
    } else {
        dbg_bytes(rte_pktmbuf_mtod(mbuf, uint8_t*), rte_pktmbuf_pkt_len(mbuf), "   " T4LIT(<<,outgoing) " " T4LIT(Emitting,outgoing) " packet on port " T4LIT(%d,port) " (" T4LIT(%d) " bytes): ", egress_port, rte_pktmbuf_pkt_len(mbuf));
        send_single_packet(pd->wrapper, egress_port, ingress_port, false, LCPARAMS_IN);
    }
}

void do_single_tx(unsigned queue_idx, unsigned pkt_idx, LCPARAMS)
{
    if (unlikely(GET_INT32_AUTO_PACKET(pd, HDR(all_metadatas), EGRESS_META_FLD) == EGRESS_DROP_VALUE)) {
        debug(" " T4LIT(XXXX,status) " " T4LIT(Dropping,status) " packet\n");
        free_packet(LCPARAMS_IN);
    } else {
        debug(" " T4LIT(<<<<,outgoing) " " T4LIT(Egressing,outgoing) " packet\n");

        int egress_port = extract_egress_port(pd);
        int ingress_port = extract_ingress_port(pd);

        send_packet(egress_port, ingress_port, LCPARAMS_IN);
    }
}

void do_single_rx(unsigned queue_idx, unsigned pkt_idx, LCPARAMS)
{
    bool got_packet = receive_packet(pkt_idx, LCPARAMS_IN);

    if (got_packet) {
        if (likely(is_packet_handled(LCPARAMS_IN))) {
            struct lcore_state state = lcdata->conf->state;
            lookup_table_t** tables = state.tables;
            parser_state_t* pstate = &(state.parser_state);

            int portid = get_portid(queue_idx, LCPARAMS_IN);

            init_parser_state(&(state.parser_state));
            handle_packet(portid, get_packet_idx(LCPARAMS_IN), STDPARAMS_IN);
            do_single_tx(queue_idx, pkt_idx, LCPARAMS_IN);
        }
    }

    main_loop_post_single_rx(got_packet, LCPARAMS_IN);
}

void do_rx(LCPARAMS)
{
    unsigned queue_count = get_queue_count(lcdata);
    for (unsigned queue_idx = 0; queue_idx < queue_count; queue_idx++) {
        main_loop_rx_group(queue_idx, LCPARAMS_IN);

        unsigned pkt_count = get_pkt_count_in_group(lcdata);
        for (unsigned pkt_idx = 0; pkt_idx < pkt_count; pkt_idx++) {
            do_single_rx(queue_idx, pkt_idx, LCPARAMS_IN);
        }
    }
}

void dpdk_main_loop()
{
    struct lcore_data lcdata_content = init_lcore_data();
    packet_descriptor_t pd_content;

    struct lcore_data* lcdata = &lcdata_content;
    packet_descriptor_t* pd = &pd_content;

    if (!lcdata->is_valid) {
        return;
    }

    init_dataplane(pd, lcdata->conf->state.tables);

    while (core_is_working(LCPARAMS_IN)) {
        main_loop_pre_rx(LCPARAMS_IN);
        do_rx(LCPARAMS_IN);
        main_loop_post_rx(LCPARAMS_IN);
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

int main(int argc, char** argv)
{
    debug("Init switch\n");

    initialize_args(argc, argv);
    initialize_nic();

    int launch_count2 = launch_count();
    for (int idx = 0; idx < launch_count2; ++idx) {
        debug("Init execution\n");

        init_tables();
        init_storage();

        init_memories();
        #ifndef T4P4S_NO_CONTROL_PLANE
            debug(" " T4LIT(::::,incoming) " Init control plane connection\n");
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
