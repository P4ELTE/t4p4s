// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include <inttypes.h>
#include <stdbool.h>

#include "aliases.h"
#include "parser.h"

#include "dataplane_lookup.h"

#if ASYNC_MODE != ASYNC_MODE_OFF
    #include "dpdk_lib_conf_async.h"
#endif

typedef struct {
    header_instance_e header;
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
    header_instance_e header_instance;
    int bytewidth;
    int var_width_field;
} header_reference_t;

#define FIELD_FIXED_WIDTH_PD(f) (f != hdr_infos[fld_infos[f].header_instance].var_width_field)
#define FIELD_FIXED_POS_PD(f)   (f <= hdr_infos[fld_infos[f].header_instance].var_width_field || hdr_infos[fld_infos[f].header_instance].var_width_field == -1)

#define FIELD_DYNAMIC_BITWIDTH_PD(pd, f) (FIELD_FIXED_WIDTH_PD(f) ? fld_infos[f].bit_width : (pd)->headers[fld_infos[f].header_instance].vw_size)
#define FIELD_DYNAMIC_BYTEOFFSET_PD(pd, f) (fld_infos[f].byte_offset + (FIELD_FIXED_POS_PD(f) ? 0 : ((pd)->headers[fld_infos[f].header_instance].vw_size / 8)))

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
    header_instance_e   type;
    void *              pointer;
    uint32_t            size;
    int                 vw_size;
    bool                was_enabled_at_initial_parse;
#ifdef T4P4S_DEBUG
    const char*         name;
#endif
} header_descriptor_t;

typedef struct {
    int current;
} pkt_header_stack_e;

typedef struct {
    packet_data_t*      data;
    void*               extract_ptr;
    header_descriptor_t headers[HEADER_COUNT+1];
    parsed_fields_t     fields;
    packet*             wrapper;

    pkt_header_stack_e  stacks[STACK_COUNT+1];

    int deparse_hdrinst_count;
    int deparse_size;
    int parsed_size;
    int payload_size;
    bool is_deparse_reordering;
    // note: it is possible to emit a header more than once; +8 is a reasonable upper limit for emits
    int header_reorder[HEADER_COUNT+8];
    uint8_t header_tmp_storage[NONMETA_HDR_TOTAL_LENGTH];

    void * control_locals;

    #if ASYNC_MODE != ASYNC_MODE_OFF
        // async functionality
        void *context;

        int port_id;
        unsigned queue_idx;
        unsigned pkt_idx;
        #if ASYNC_MODE == ASYNC_MODE_PD
            int program_restore_phase;
        #endif
    #endif

    #ifdef T4P4S_DEBUG
        bool is_egress_port_set;
    #endif
} packet_descriptor_t;


#define SHORT_STDPARAMS packet_descriptor_t* pd, lookup_table_t** tables
#define STDPARAMS       SHORT_STDPARAMS, parser_state_t* pstate
#define LCPARAMS        struct lcore_data* lcdata, packet_descriptor_t* pd

#define SHORT_STDPARAMS_IN pd, tables
#define STDPARAMS_IN       SHORT_STDPARAMS_IN, pstate
#define LCPARAMS_IN        lcdata, pd


uint8_t* get_fld_pointer(const packet_descriptor_t* pd, field_instance_e fld);

void activate_hdr(header_instance_e hdr, packet_descriptor_t* pd);
void deactivate_hdr(header_instance_e hdr, packet_descriptor_t* pd);
bool is_header_valid(header_instance_e, packet_descriptor_t*);

void stk_next(header_stack_e stk, packet_descriptor_t* pd);
header_instance_e stk_at_idx(header_stack_e stk, int idx, packet_descriptor_t* pd);
header_instance_e stk_current(header_stack_e stk, packet_descriptor_t* pd);
field_instance_e stk_start_fld(header_instance_e hdr);

void do_assignment(header_instance_e dst_hdr, header_instance_e src_hdr, SHORT_STDPARAMS);
void set_hdr_valid(header_instance_e hdr, SHORT_STDPARAMS);
void set_hdr_invalid(header_instance_e hdr, SHORT_STDPARAMS);

// note: currently implemented in dataplane_deparse.c.py
bool is_packet_dropped(packet_descriptor_t* pd);
