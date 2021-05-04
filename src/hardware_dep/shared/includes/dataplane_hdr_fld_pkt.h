// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include <inttypes.h>
#include <stdbool.h>

#include "aliases.h"
#include "parser.h"

typedef struct {
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

typedef struct {
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

typedef struct {
    header_instance_t   type;
    void *              pointer;
    uint32_t            length;
    int                 var_width_field_bitwidth;
    bool                was_enabled_at_initial_parse;
#ifdef T4P4S_DEBUG
    const char*         name;
#endif
} header_descriptor_t;

typedef struct {
    int current;
} pkt_header_stack_t;

typedef struct {
    packet_data_t*      data;
    void*               extract_ptr;
    header_descriptor_t headers[HEADER_COUNT+1];
    parsed_fields_t     fields;
    packet*             wrapper;

    pkt_header_stack_t  stacks[STACK_COUNT+1];

    int emit_hdrinst_count;
    int emit_headers_length;
    int parsed_length;
    int payload_length;
    bool is_emit_reordering;
    // note: it is possible to emit a header more than once; +8 is a reasonable upper limit for emits
    int header_reorder[HEADER_COUNT+8];
    uint8_t header_tmp_storage[NONMETA_HDR_TOTAL_LENGTH];

    void * control_locals;

    // async functionality
    void *context;

    int port_id;
    unsigned queue_idx;
    unsigned pkt_idx;
    int program_state;
} packet_descriptor_t;


void activate_hdr(header_instance_t hdr, packet_descriptor_t* pd);
void deactivate_hdr(header_instance_t hdr, packet_descriptor_t* pd);

void stk_next(header_stack_t stk, packet_descriptor_t* pd);
header_instance_t stk_at_idx(header_stack_t stk, int idx, packet_descriptor_t* pd);
header_instance_t stk_current(header_stack_t stk, packet_descriptor_t* pd);
field_instance_t stk_start_fld_idx(header_instance_t hdr);

#define clear_pd_states(pd)\
                { \
                    pd->context = NULL; \
                    pd->program_state = 0; \
                }

