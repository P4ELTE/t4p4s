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
    unsigned id;
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

typedef struct p4_register_s {
    char* name;
    uint8_t width;
    int size;
    lock *locks;
    uint8_t **values;
} p4_register_t;
/* /usr/include/x86_64-linux-gnu/sys/types.h:205:13: note: previous declaration of ‘register_t’ was here
   typedef int register_t __attribute__ ((__mode__ (__word__))); */

typedef struct field_reference_s {
    header_instance_t header;
    int meta;
    int bitwidth;
    int bytewidth;
    int bytecount;
    int bitoffset;
    int byteoffset;
    uint32_t mask;
    int fixed_width;     // Determines whether the field has a fixed width
    uint8_t* byte_addr;  // Pointer to the byte containing the first bit of the field in the packet
} field_reference_t;

typedef struct header_reference_s {
    header_instance_t header_instance;
    int bytewidth;
    int var_width_field;
} header_reference_t;

#define FIELD_FIXED_WIDTH(f) (f != header_instance_var_width_field[field_instance_header[f]])
#define FIELD_FIXED_POS(f)   (f <= header_instance_var_width_field[field_instance_header[f]] || header_instance_var_width_field[field_instance_header[f]] == -1)

#define FIELD_DYNAMIC_BITWIDTH(pd, f) (FIELD_FIXED_WIDTH(f) ? field_instance_bit_width[f] : (pd)->headers[field_instance_header[f]].var_width_field_bitwidth)
#define FIELD_DYNAMIC_BYTEOFFSET(pd, f) (field_instance_byte_offset_hdr[f] + (FIELD_FIXED_POS(f) ? 0 : ((pd)->headers[field_instance_header[f]].var_width_field_bitwidth / 8)))

#define field_desc(pd, f) (field_reference_t) \
               { \
                 .header     = field_instance_header[f], \
                 .meta       = header_instance_is_metadata[field_instance_header[f]], \
                 .bitwidth   =   FIELD_DYNAMIC_BITWIDTH(pd, f), \
                 .bytewidth  =  (FIELD_DYNAMIC_BITWIDTH(pd, f) + 7) / 8, \
                 .bytecount  = ((FIELD_DYNAMIC_BITWIDTH(pd, f) + 7 + field_instance_bit_offset[f]) / 8), \
                 .bitoffset  = field_instance_bit_offset[f], \
                 .byteoffset = FIELD_DYNAMIC_BYTEOFFSET(pd, f), \
                 .mask       = field_instance_mask[f], \
                 .fixed_width= FIELD_FIXED_WIDTH(f), \
                 .byte_addr  = (((uint8_t*)(pd)->headers[field_instance_header[f]].pointer)+(FIELD_DYNAMIC_BYTEOFFSET(pd, f))), \
               }

#define header_info(h) (header_reference_t) \
               { \
                 .header_instance = h, \
                 .bytewidth       = header_instance_byte_width[h], \
                 .var_width_field  = header_instance_var_width_field[h], \
               }

typedef struct header_descriptor_s {
    header_instance_t   type;
    void *              pointer;
    uint32_t            length;
    int                 var_width_field_bitwidth;
} header_descriptor_t;

typedef struct packet_descriptor_s {
    void *              data;
    header_descriptor_t headers[HEADER_INSTANCE_COUNT+1];
    parsed_fields_t     fields;
    packet *            wrapper;
    uint8_t             dropped;
} packet_descriptor_t;

//=============================================================================
// Callbacks

extern lookup_table_t table_config[];
extern counter_t counter_config[];
extern p4_register_t register_config[];

void init_dataplane(packet_descriptor_t* packet, lookup_table_t** tables);
void handle_packet(packet_descriptor_t* packet, lookup_table_t** tables);

#endif
