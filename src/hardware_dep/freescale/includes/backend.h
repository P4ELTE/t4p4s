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
#ifndef __BACKEND_H_
#define __BACKEND_H_

#include "ctrl_plane_backend.h"
#include "dataplane.h"
#include <stdio.h>
#include <stdlib.h>
#include "freescale_primitives.h"

typedef struct packet_descriptor_s packet_descriptor_t;
typedef struct header_descriptor_s header_descriptor_t;
typedef struct header_reference_s  header_reference_t;
typedef struct field_reference_s   field_reference_t;

//=============================================================================
// General

uint8_t       initialize   (int argc, char **argv);
int           launch       (void);

//=============================================================================
// Table mgmt

void        exact_create (lookup_table_t* t, int socketid);
void          lpm_create (lookup_table_t* t, int socketid);
void      ternary_create (lookup_table_t* t, int socketid);

void    table_setdefault (lookup_table_t* t,                              uint8_t* value);

void           exact_add (lookup_table_t* t, uint8_t* key,                uint8_t* value);
void             lpm_add (lookup_table_t* t, uint8_t* key, uint8_t depth, uint8_t* value);
void         ternary_add (lookup_table_t* t, uint8_t* key, uint8_t* mask, uint8_t* value);

uint8_t*    exact_lookup (lookup_table_t* t, uint8_t* key);
uint8_t*      lpm_lookup (lookup_table_t* t, uint8_t* key);
uint8_t*  ternary_lookup (lookup_table_t* t, uint8_t* key);

//=============================================================================
// Primitive actions


void     increase_counter (int id, int index);
void        read_register (int id, int index, uint8_t* dst);
void       write_register (int id, int index, uint8_t* src);

void push (packet_descriptor_t* p, header_stack_t h);
void pop  (packet_descriptor_t* p, header_stack_t h);

void add_header             (packet_descriptor_t* p, header_reference_t h);
void remove_header          (packet_descriptor_t* p, header_reference_t h);
void drop                   (packet_descriptor_t* p);
void generate_digest        (backend bg, char* name, int receiver, struct type_field_list* digest_field_list);
void no_op                  (void);

//copy_header
//void set_field_to_hash_index(packet* p, field* f, field* flc, int base, int size);/
//void truncate_pkg           (packet* p, unsigned length);
//void push                   (packet* p, header_idx* hdr_array);
//void push_n                 (packet* p, header_idx* hdr_array, unsigned count);
//void pop                    (packet* p, header_idx* hdr_array);
//void pop_n                  (packet* p, header_idx* hdr_array, unsigned count);
//count
//meter
//resubmit
//recirculate
//clone_ingress_pkt_to_ingress
//clone_egress_pkt_to_ingress
//clone_ingress_pkt_to_egress
//clone_egress_pkt_to_egress

#endif // __BACKEND_H_
