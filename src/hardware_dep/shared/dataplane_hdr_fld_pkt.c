// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "dataplane_hdr_fld_pkt.h"

void activate_hdr(header_instance_t hdr, packet_descriptor_t* pd) {
    pd->headers[hdr].pointer = (pd->header_tmp_storage + hdr_infos[hdr].byte_offset);
    pd->is_emit_reordering = true;
}

void deactivate_hdr(header_instance_t hdr, packet_descriptor_t* pd) {
    pd->headers[hdr].pointer = 0;
    pd->is_emit_reordering = true;
}

void stk_next(header_stack_t stk, packet_descriptor_t* pd) {
    ++pd->stacks[stk].current;
}

header_instance_t stk_at_idx(header_stack_t stk, int idx, packet_descriptor_t* pd) {
    return stk_infos[stk].start_hdr + idx;
}

header_instance_t stk_current(header_stack_t stk, packet_descriptor_t* pd) {
    return stk_at_idx(stk, pd->stacks[stk].current, pd);
}

field_instance_t stk_start_fld(header_instance_t hdr) {
    stk_info_t infos = stk_infos[hdr];
    return infos.start_fld + (hdr - infos.start_hdr) * infos.fld_count;
}

uint8_t* get_fld_pointer(const packet_descriptor_t* pd, field_instance_t fld) {
    fld_info_t info = fld_infos[fld];
    header_instance_t hdr = info.header_instance;

    field_instance_t vw_fld = hdr_infos[hdr].var_width_field;

    uint8_t* ptr = (uint8_t*)pd->headers[hdr].pointer;
    int static_offset = info.byte_offset;
    bool is_dyn = fld <= vw_fld || vw_fld == -1;
    int dyn_offset = is_dyn ? 0 : pd->headers[hdr].var_width_field_bitwidth / 8;
    return ptr + static_offset + dyn_offset;
}
