#[ // SPDX-License-Identifier: Apache-2.0
#[ // Copyright 2021 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_type

#[ #include "common.h"
#[ #include "util_debug.h"
#[

#{ void do_assignment(header_instance_t dst_hdr, header_instance_t src_hdr, SHORT_STDPARAMS) {
#{     if (likely(is_header_valid(src_hdr, pd))) {
#{         if (unlikely(!is_header_valid(dst_hdr, pd))) {
#[             activate_hdr(dst_hdr, pd);
#}         }
#[         memcpy(pd->headers[dst_hdr].pointer, pd->headers[src_hdr].pointer, hdr_infos[src_hdr].byte_width);
#[         dbg_bytes(pd->headers[dst_hdr].pointer, hdr_infos[src_hdr].byte_width, "    : Set " T4LIT(%s,header) "/" T4LIT(%dB) " = " T4LIT(%s,header) " = ", hdr_infos[dst_hdr].name, hdr_infos[src_hdr].byte_width, hdr_infos[src_hdr].name);
#[     } else {
#[         debug("   :: Set header " T4LIT(%s,header) "/" T4LIT(%dB) " = " T4LIT(invalid,status) " from " T4LIT(%s,header) "/" T4LIT(%dB) "\n", hdr_infos[dst_hdr].name, hdr_infos[dst_hdr].byte_width, hdr_infos[src_hdr].name, hdr_infos[src_hdr].byte_width);
#[         deactivate_hdr(dst_hdr, pd);
#}     }
#} }
#[

#{ void set_hdr_valid(header_instance_t hdr, SHORT_STDPARAMS) {
#{     if (likely(pd->headers[hdr].pointer == NULL)) {
#[        activate_hdr(hdr, pd);
#[        pd->is_emit_reordering = true;
#[        debug("   :: Set header " T4LIT(%s,header) "/" T4LIT(%dB) " = $$[success]{}{valid}\n", hdr_infos[hdr].name, pd->headers[hdr].length);
#[     } else {
#[        debug("   " T4LIT(!!,warning) " Trying to set header " T4LIT(%s,header) " to $$[success]{}{valid}, but it is already $$[success]{}{valid}\n", hdr_infos[hdr].name);
#}     }
#} }
#[

#{ void set_hdr_invalid(header_instance_t hdr, SHORT_STDPARAMS) {
#{     if (likely(pd->headers[hdr].pointer != NULL)) {
#[        pd->headers[hdr].pointer = NULL;
#[        pd->is_emit_reordering = true;
#[        debug("   :: Set header " T4LIT(%s,header) "/" T4LIT(%dB) " = $$[status]{}{invalid}\n", hdr_infos[hdr].name, pd->headers[hdr].length);
#[     } else {
#[        debug("   " T4LIT(!!,warning) " Trying to set header " T4LIT(%s,header) " to $$[status]{}{invalid}, but it is already $$[status]{}{invalid}\n", hdr_infos[hdr].name);
#}     }
#} }
#[

