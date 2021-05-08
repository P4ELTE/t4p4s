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


void initialize_args(int argc, char **argv);
void initialize_nic();
int init_tables();
int init_memories();

int flush_tables();

int launch_count();
void t4p4s_abnormal_exit(int retval, int idx);
void t4p4s_pre_launch(int idx);
void t4p4s_post_launch(int idx);
int t4p4s_normal_exit();

void init_control_plane();

// defined separately for each example
bool core_is_working(LCPARAMS);
bool receive_packet(unsigned pkt_idx, LCPARAMS);
void free_packet(LCPARAMS);
bool is_packet_handled(LCPARAMS);
void init_storage();
void main_loop_pre_rx(LCPARAMS);
void main_loop_post_rx(bool, LCPARAMS);
void main_loop_post_single_rx(bool got_packet, LCPARAMS);
uint32_t get_portid(unsigned queue_idx, LCPARAMS);
void main_loop_rx_group(unsigned queue_idx, LCPARAMS);
unsigned get_pkt_count_in_group();
unsigned get_queue_count();
void send_single_packet(packet* pkt, int egress_port, int ingress_port, bool send_clone, LCPARAMS);
void send_broadcast_packet(int egress_port, int ingress_port, LCPARAMS);
struct lcore_data init_lcore_data();
packet* clone_packet(packet* pd, struct rte_mempool* mempool);
void init_parser_state(parser_state_t*);
void init_table_default_actions();
int get_packet_idx(LCPARAMS);

uint32_t get_port_mask();
uint8_t get_port_count();

void t4p4s_print_global_stats();
void t4p4s_print_per_packet_stats();
void t4p4s_init_global_stats();
void t4p4s_init_per_packet_stats();
bool check_controlflow_requirements(fake_cmd_t cmd);
