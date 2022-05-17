// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "dataplane_hdr_fld_pkt.h"
#include "util_debug.h"

void activate_hdr(header_instance_e hdr, packet_descriptor_t* pd) {
    pd->headers[hdr].pointer = (pd->header_tmp_storage + hdr_infos[hdr].byte_offset);
    pd->is_deparse_reordering = true;

    // Note: fields are not initialized because:
    // ""... reading an uninitialized header produces an undefined value, even if the header is itself valid."
}

void deactivate_hdr(header_instance_e hdr, packet_descriptor_t* pd) {
    pd->headers[hdr].pointer = 0;
    pd->is_deparse_reordering = true;
}

void stk_next(header_stack_e stk, packet_descriptor_t* pd) {
    ++pd->stacks[stk].current;
}

header_instance_e stk_at_idx(header_stack_e stk, int idx, packet_descriptor_t* pd) {
    return stk_infos[stk].start_hdr + idx;
}

header_instance_e stk_current(header_stack_e stk, packet_descriptor_t* pd) {
    return stk_at_idx(stk, pd->stacks[stk].current, pd);
}

field_instance_e stk_start_fld(header_instance_e hdr) {
    #ifdef T4P4S_DEBUG
        // note: this should never happen
        if (hdr_infos[hdr].stack_idx == NO_STACK_PRESENT) {
            debug(" " T4LIT(!!!!,error) " Header #%d (%s) is supposed to be a stack, but it is not\n", hdr, hdr_infos[hdr].name);
        }
    #endif

    return hdr_infos[hdr].first_fld;
}

int get_vw_fld_offset(const packet_descriptor_t* pd, header_instance_e hdr, field_instance_e fld, field_instance_e vw_fld) {
    bool is_dyn = fld <= vw_fld || vw_fld == NO_VW_FIELD_PRESENT;
    return is_dyn ? 0 : pd->headers[hdr].vw_size / 8;
}

uint8_t* get_fld_pointer(const packet_descriptor_t* pd, field_instance_e fld) {
    fld_info_t info = fld_infos[fld];
    header_instance_e hdr = info.header_instance;
    field_instance_e vw_fld = hdr_infos[hdr].var_width_field;

    uint8_t* hdrptr = pd->headers[hdr].pointer;
    return hdrptr + info.byte_offset + get_vw_fld_offset(pd, hdr, fld, vw_fld);
}
