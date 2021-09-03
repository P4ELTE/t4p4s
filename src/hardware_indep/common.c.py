#[ // SPDX-License-Identifier: Apache-2.0
#[ // Copyright 2021 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_type

#[ #include "common.h"
#[ #include "util_debug.h"
#[

#{ void do_assignment(header_instance_e dst_hdr, header_instance_e src_hdr, SHORT_STDPARAMS) {
#{     if (likely(is_header_valid(src_hdr, pd))) {
#{         if (unlikely(!is_header_valid(dst_hdr, pd))) {
#[             activate_hdr(dst_hdr, pd);
#}         }
#[         int copy_len = hdr_infos[src_hdr].byte_width + pd->headers[src_hdr].vw_size / 8;
#[         memcpy(pd->headers[dst_hdr].pointer, pd->headers[src_hdr].pointer, copy_len);
#[         dbg_bytes(pd->headers[dst_hdr].pointer, copy_len, "    " T4LIT(=,field) " Set " T4LIT(%s,header) "/" T4LIT(%dB) " = " T4LIT(%s,header) " = ",
#[                   hdr_infos[dst_hdr].name, hdr_infos[src_hdr].byte_width, hdr_infos[src_hdr].name);
#[     } else {
#[         debug("    " T4LIT(=,status) " Set header " T4LIT(%s,header) "/" T4LIT(%dB) " = " T4LIT(invalid,status) " from " T4LIT(%s,header) "/" T4LIT(%dB) "\n",
#[               hdr_infos[dst_hdr].name, hdr_infos[dst_hdr].byte_width, hdr_infos[src_hdr].name, hdr_infos[src_hdr].byte_width);
#[         deactivate_hdr(dst_hdr, pd);
#}     }
#} }
#[

#{ void debug_validity_info_msg(bool is_valid, const uint8_t*const ptr, header_instance_e hdr, SHORT_STDPARAMS) {
#[     bool is_ok = is_header_valid(hdr, pd) == is_valid;
#[     const char* status_txt = is_valid ? T4LIT(valid,success) : T4LIT(invalid,status);
#{     if (is_ok) {
#[        debug("   :: Set header " T4LIT(%s,header) "/" T4LIT(%d) "+" T4LIT(%d) "B = %s\n",
#[              hdr_infos[hdr].name, pd->headers[hdr].size, pd->headers[hdr].vw_size/8, status_txt);
#[     } else {
#[        debug("   " T4LIT(!!,warning) " Trying to set header " T4LIT(%s,header) " to %s but it is already %s\n", hdr_infos[hdr].name, status_txt, status_txt);
#}     }
#} }
#[


#{ void set_hdr_valid(header_instance_e hdr, SHORT_STDPARAMS) {
#{     if (likely(!is_header_valid(hdr, pd))) {
#[        activate_hdr(hdr, pd);
#[        pd->is_deparse_reordering = true;
#}     }
#[     debug_validity_info_msg(true, pd->headers[hdr].pointer, hdr, SHORT_STDPARAMS_IN);
#} }
#[

#{ void set_hdr_invalid(header_instance_e hdr, SHORT_STDPARAMS) {
#{     if (likely(is_header_valid(hdr, pd))) {
#[        pd->headers[hdr].pointer = NULL;
#[        pd->is_deparse_reordering = true;
#}     }
#[     debug_validity_info_msg(false, pd->headers[hdr].pointer, hdr, SHORT_STDPARAMS_IN);
#} }
#[


def remove_prefix(txt, prefix):
    return txt[len(prefix):] if txt.startswith(prefix) else txt

def remove_suffix(txt, suffix):
    return txt[:-len(suffix)] if txt.endswith(suffix) else txt

def short_name(name):
    return remove_suffix(name, '_t')

for ee in hlir.errors + hlir.enums:
    kind = 'enum' if ee.node_type == 'Type_Enum' else 'error'
    name = short_name(ee.c_name)
    #{ const char* ${kind}_value_names_${ee.name}[${len(ee.members)}] = {
    for m in ee.members:
        name = remove_prefix(m.c_name, f'{kind}_{ee.name}_')
        #[     "$name",
    #} };
    #[
#[
