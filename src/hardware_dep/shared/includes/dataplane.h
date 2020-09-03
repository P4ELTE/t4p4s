// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include "aliases.h"
#include "parser.h"

enum lookup_t {
    LOOKUP_none,
    // TODO make sure all of these names are OK
    LOOKUP_exact,
    LOOKUP_LPM,
    LOOKUP_TERNARY,
};

#define LOOKUP_EXACT   0
#define LOOKUP_LPM     1
#define LOOKUP_TERNARY 2

struct type_field_list {
    uint8_t fields_quantity;
    uint8_t** field_offsets;
    uint8_t* field_widths;
};

typedef struct lookup_table_entry_info_s {
    int entry_count;

    uint8_t key_size;

    // entry size == val_size + validity_size + state_size
    uint8_t entry_size;
    uint8_t action_size;
    uint8_t validity_size;
    uint8_t state_size;
} lookup_table_entry_info_t;

typedef struct lookup_table_s {
    char* name;
    unsigned id;
    uint8_t type;

    int min_size;
    int max_size;

    void* default_val;
    void* table;

    int socketid;
    int instance;

    lookup_table_entry_info_t entry;
#ifdef T4P4S_DEBUG
    int init_entry_count;
#endif
} lookup_table_t;

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

#define FIELD_FIXED_WIDTH_PD(f) (f != hdr_infos[fld_infos[f].header_instance].var_width_field)
#define FIELD_FIXED_POS_PD(f)   (f <= hdr_infos[fld_infos[f].header_instance].var_width_field || hdr_infos[fld_infos[f].header_instance].var_width_field == -1)

#define FIELD_DYNAMIC_BITWIDTH_PD(pd, f) (FIELD_FIXED_WIDTH_PD(f) ? fld_infos[f].bit_width : (pd)->headers[fld_infos[f].header_instance].var_width_field_bitwidth)
#define FIELD_DYNAMIC_BYTEOFFSET_PD(pd, f) (fld_infos[f].byte_offset + (FIELD_FIXED_POS_PD(f) ? 0 : ((pd)->headers[fld_infos[f].header_instance].var_width_field_bitwidth / 8)))

#define field_desc(pd, f) ((field_reference_t) \
               { \
                 .header     = fld_infos[f].header_instance, \
                 .meta       = hdr_infos[fld_infos[f].header_instance].is_metadata, \
                 .bitwidth   =   FIELD_DYNAMIC_BITWIDTH_PD(pd, f), \
                 .bytewidth  =  (FIELD_DYNAMIC_BITWIDTH_PD(pd, f) + 7) / 8, \
                 .bytecount  = ((FIELD_DYNAMIC_BITWIDTH_PD(pd, f) + 7 + fld_infos[f].bit_offset) / 8), \
                 .bitoffset  = fld_infos[f].bit_offset, \
                 .byteoffset = FIELD_DYNAMIC_BYTEOFFSET_PD(pd, f), \
                 .mask       = fld_infos[f].mask, \
                 .fixed_width= FIELD_FIXED_WIDTH_PD(f), \
                 .byte_addr  = (((uint8_t*)(pd)->headers[fld_infos[f].header_instance].pointer)+(FIELD_DYNAMIC_BYTEOFFSET_PD(pd, f))), \
               })

#define header_info(h) (header_reference_t) \
               { \
                 .header_instance = h, \
                 .bytewidth       = hdr_infos[h].byte_width, \
                 .var_width_field = hdr_infos[h].var_width_field, \
               }

typedef struct header_descriptor_s {
    header_instance_t   type;
    void *              pointer;
    uint32_t            length;
    int                 var_width_field_bitwidth;
    bool                was_enabled_at_initial_parse;
#ifdef T4P4S_DEBUG
    char*               name;
#endif
} header_descriptor_t;

typedef struct packet_descriptor_s {
    packet_data_t*      data;
    header_descriptor_t headers[HEADER_COUNT+1];
    parsed_fields_t     fields;
    packet*             wrapper;

    int emit_hdrinst_count;
    int emit_headers_length;
    int parsed_length;
    int payload_length;
    bool is_emit_reordering;
    // note: it is possible to emit a header more than once; +8 is a reasonable upper limit for emits
    int header_reorder[HEADER_COUNT+8];
    uint8_t header_tmp_storage[NONMETA_HDR_TOTAL_LENGTH];

    void * control_locals;
} packet_descriptor_t;

//=============================================================================
// Callbacks

extern lookup_table_t table_config[];

void init_dataplane(packet_descriptor_t* packet, lookup_table_t** tables);
void handle_packet(packet_descriptor_t* packet, lookup_table_t** tables, parser_state_t* pstate, uint32_t portid);
