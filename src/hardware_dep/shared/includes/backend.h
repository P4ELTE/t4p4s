// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include "ctrl_plane_backend.h"
#include "dataplane.h"
#include "util_debug.h"
#include "util_packet.h"
#include <string.h>


//=============================================================================
// General

void          initialize (int argc, char **argv);
int           launch     (void);

//=============================================================================
// Table mgmt

void        exact_create (lookup_table_t* t, int socketid);
void          lpm_create (lookup_table_t* t, int socketid);
void      ternary_create (lookup_table_t* t, int socketid);

void    table_setdefault (lookup_table_t* t,                              base_table_action_t* entry);

void           exact_add (lookup_table_t* t, uint8_t* key,                base_table_action_t* entry);
void             lpm_add (lookup_table_t* t, uint8_t* key, uint8_t depth, base_table_action_t* entry);
void         ternary_add (lookup_table_t* t, uint8_t* key, uint8_t* mask, base_table_action_t* entry);

uint8_t*    exact_lookup (lookup_table_t* t, uint8_t* key);
uint8_t*      lpm_lookup (lookup_table_t* t, uint8_t* key);
uint8_t*  ternary_lookup (lookup_table_t* t, uint8_t* key);

//=============================================================================
// Calculations

uint16_t calculate_csum16(const void* buf, uint16_t length);

uint32_t packet_size(packet_descriptor_t* pd);

//=============================================================================
// Primitive actions

#include "dpdk_primitives.h" // field manipulation primitives are implemented as macros

void increase_counter(int id, int index);
void    read_register(int id, int index, uint8_t* dst);
void   write_register(int id, int index, uint8_t* src);

void add_header     (packet_descriptor_t* p, header_reference_t h);
void remove_header  (packet_descriptor_t* p, header_reference_t h);
void drop           (packet_descriptor_t* p);
void generate_digest(ctrl_plane_backend bg, char* name, int receiver, struct type_field_list* digest_field_list);

//=============================================================================
// Misc

bool is_header_valid(header_instance_e hdr, packet_descriptor_t* pd);
