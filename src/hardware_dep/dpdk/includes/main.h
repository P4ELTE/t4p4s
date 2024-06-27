// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include "util_packet.h"
#include "dpdk_lib.h"

#ifndef T4P4S_NIC_VARIANT
#error The NIC variant is undefined
#endif

#ifdef T4P4S_SUPPRESS_EAL
    #include <unistd.h>
    #include <stdio.h>
#endif


#if ASYNC_MODE != ASYNC_MODE_OFF
    // defined in main_async.c
    extern void async_handle_packet(unsigned port_id, int pkt_idx, packet_handler_t handler_function, LCPARAMS);
    extern void main_loop_async(LCPARAMS);
    extern void main_loop_fake_crypto(LCPARAMS);
#endif


TODO_INLINING void initialize_args(int argc, char **argv);
TODO_INLINING int init_tables();
TODO_INLINING int init_memories();
TODO_INLINING void init_parser_state(parser_state_t*);
TODO_INLINING void init_table_default_actions();
TODO_INLINING void init_control_plane();

int flush_tables();

packet* clone_packet(packet* pd, struct rte_mempool* mempool);

void t4p4s_print_global_stats();
void t4p4s_init_global_stats();

int pick_random_port();
