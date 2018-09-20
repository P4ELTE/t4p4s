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
#include "util.h"
#include <string.h>

#ifdef T4P4S_DEBUG
	#define __SHORTFILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
	#define SHORTEN(str, len) ((strlen(str) <= (len)) ? (str) : ((str) + (strlen(str) - len)))

	#define lcore_debug(M, ...)   fprintf(stderr, "%11.11s@%4d [CORE" T4LIT(%2d,core) "@" T4LIT(%d,socket) "] " M "", SHORTEN(__SHORTFILENAME__, 13), __LINE__, (int)(rte_lcore_id()), rte_lcore_to_socket_id(rte_lcore_id()), ##__VA_ARGS__)
	#define no_core_debug(M, ...) fprintf(stderr, "%11.11s@%4d [NO-CORE ] " M "", SHORTEN(__SHORTFILENAME__, 13), __LINE__, ##__VA_ARGS__)

	#include <pthread.h>
	pthread_mutex_t dbg_mutex;

	#define debug_printf(M, ...)   ((rte_lcore_id() == UINT_MAX) ? no_core_debug(M, ##__VA_ARGS__) : lcore_debug(M, ##__VA_ARGS__)); \

	#define debug(M, ...) \
	    { \
		    pthread_mutex_lock(&dbg_mutex); \
			debug_printf(M, ##__VA_ARGS__); \
		    pthread_mutex_unlock(&dbg_mutex); \
		}

#else
	#define debug(M, ...)
#endif

typedef struct packet_descriptor_s packet_descriptor_t;
typedef struct header_descriptor_s header_descriptor_t;
typedef struct header_reference_s  header_reference_t;
typedef struct field_reference_s   field_reference_t;


struct uint8_buffer_s {
	   int      buffer_size;
	   uint8_t* buffer;
};

//=============================================================================
// General

void          initialize (int argc, char **argv);
int           launch     (void);

//=============================================================================
// Table mgmt

typedef struct lookup_table_s lookup_table_t;

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
// Calculations

uint16_t calculate_csum16(const void* buf, uint16_t length);

uint32_t packet_length(packet_descriptor_t* pd);

//=============================================================================
// Primitive actions

#include "dpdk_primitives.h" // field manipulation primitives are implemented as macros

void     increase_counter (int id, int index);
void        read_register (int id, int index, uint8_t* dst);
void       write_register (int id, int index, uint8_t* src);

void push (packet_descriptor_t* p, header_stack_t h);
void pop  (packet_descriptor_t* p, header_stack_t h);

void add_header             (packet_descriptor_t* p, header_reference_t h);
void remove_header          (packet_descriptor_t* p, header_reference_t h);
void drop                   (packet_descriptor_t* p);
void generate_digest        (ctrl_plane_backend bg, char* name, int receiver, struct type_field_list* digest_field_list);
void no_op                  ();

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
