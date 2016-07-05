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
#ifndef DATAPLANE_H
#define DATAPLANE_H

#include <inttypes.h>
#include "aliases.h"
#include "parser.h"
#include "vector.h"

#define LOOKUP_EXACT   0
#define LOOKUP_LPM     1
#define LOOKUP_TERNARY 2

struct type_field_list {
    uint8_t fields_quantity;
    uint8_t** field_offsets;
    uint8_t* field_widths;
};

typedef struct lookup_table_s {
    char* name;
    uint8_t type;
    uint8_t key_size;
    uint8_t val_size;
    int min_size;
    int max_size;
    int counter;
    void* default_val;
    void* table;
    int socketid;
    int instance;
} lookup_table_t;

typedef struct counter_s {
    char* name;
    uint8_t type;
    uint8_t min_width;
    int size;
    uint8_t saturating;
    vector_t *values; //rte_atomic32_t *cnt; // volatile ints
    int socketid;
} counter_t;

typedef struct field_reference_s {
    header_instance_t header;
    int meta;
    int bitwidth;
    int bytewidth;
    int bytecount;
    int bitoffset;
    int byteoffset;
    uint32_t mask;
} field_reference_t;

typedef struct header_reference_s {
    header_instance_t header_instance;
    int bytewidth;
} header_reference_t;

#define field_desc(f) (field_reference_t) \
               { \
                 .header     = field_instance_header[f], \
                 .meta       = header_instance_is_metadata[field_instance_header[f]], \
                 .bitwidth   = field_instance_bit_width[f], \
                 .bytewidth  = (field_instance_bit_width[f]+7)/8, \
                 .bytecount  = (field_instance_bit_offset[f]+field_instance_bit_width[f]+7)/8, \
                 .bitoffset  = field_instance_bit_offset[f], \
                 .byteoffset = field_instance_byte_offset_hdr[f], \
                 .mask       = field_instance_mask[f] \
               }

#define header_desc(h) (header_reference_t) \
               { \
                 .header_instance = h, \
                 .bytewidth       = header_instance_byte_width[h], \
               }

typedef struct header_descriptor_s {
    header_instance_t   type;
    void *              pointer;
    uint32_t            length;
} header_descriptor_t;

typedef struct packet_descriptor_s {
    void *              data;
    header_descriptor_t headers[HEADER_INSTANCE_COUNT+1];
    packet *            wrapper;
} packet_descriptor_t;

//=============================================================================
// Callbacks

extern lookup_table_t table_config[];
extern counter_t counter_config[];

void init_dataplane(packet_descriptor_t* packet, lookup_table_t** tables);
void handle_packet(packet_descriptor_t* packet, lookup_table_t** tables);

#endif
