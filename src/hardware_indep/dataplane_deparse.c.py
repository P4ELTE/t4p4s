# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_declaration, format_statement, format_expr, format_type, gen_format_type, get_method_call_env
from compiler_log_warnings_errors import addError, addWarning
from compiler_common import types, generate_var_name, get_hdrfld_name, unique_everseen

#[ #include "gen_include.h"
#[ #include "dataplane_impl.h"
#[ #include "dataplane.h"
#[ #include "dataplane_stages.h"
#[ #include "hdr_fld.h"
#[ #include "hdr_fld_sprintf.h"

# TODO make this an import from hardware_indep
#[ #include "dpdk_smem.h"


longest_hdr_name_len = max({len(h.name) for h in hlir.header_instances if not h.urtype.is_metadata if not h.is_local if not h.is_skipped})

pkt_name_indent = " " * longest_hdr_name_len

#{ void print_headers(SHORT_STDPARAMS) {
#{     #ifdef T4P4S_DEBUG
#[         int skips = 0;
#{         for (int i = 0; i < pd->deparse_hdrinst_count; ++i) {
#[             header_descriptor_t hdr = pd->headers[pd->header_reorder[i]];
#[             if (unlikely(hdr.pointer == NULL))    ++skips;
#}         }
#[         int deparses = pd->deparse_hdrinst_count - skips;
#[         debug(" :::: Deparse reordering: preparing " T4LIT(%d) " header%s for storage\n",
#[               deparses, deparses != 1 ? "s" : "");
#}     #endif
#} }
#[

#{ void store_headers_for_deparse(SHORT_STDPARAMS) {
#[     pd->deparse_size = 0;
#{     for (int i = 0; i < pd->deparse_hdrinst_count; ++i) {
#[         header_descriptor_t* hdr = &(pd->headers[pd->header_reorder[i]]);
#[
#{         if (unlikely(hdr->pointer == NULL)) {
#{             #ifdef T4P4S_DEBUG
#{                 if (hdr->was_enabled_at_initial_parse) {
#[                     debug("        : -" T4LIT(#%02d ,status) "$$[status][%]{longest_hdr_name_len+1}{s}$$[status]{}{/%02dB} (invalidated)\n",
#[                           hdr->type + 1, hdr->name, hdr->size);
#}                 }
#}             #endif
#[             continue;
#}         }
#[
#[         bool at_init = likely(hdr->was_enabled_at_initial_parse);
#[
#{         if (at_init) {
#[             memcpy(pd->header_tmp_storage + hdr_infos[hdr->type].byte_offset, hdr->pointer, hdr->size);
#}         }
#[
#{         #ifdef T4P4S_DEBUG
#[             char fields_txt[4096];
#[             debug("        : %s" T4LIT(#%02d) " $$[header][%]{longest_hdr_name_len}{s}/$${}{%02dB} = %s\n",
#[                   at_init ? " " : "+", hdr->type + 1, hdr->name, hdr->size,
#[                   hdr->pointer == NULL ? T4LIT((invalid),warning) " " : sprintf_hdr(fields_txt, pd, hdr));
#}         #endif
#[
#[         pd->deparse_size += hdr->size;
#}     }
#} }
#[

#[ void resize_packet_on_deparse(SHORT_STDPARAMS)
#{ {
#[     int old_size = packet_size(pd);
#[     int new_size = pd->deparse_size + pd->payload_size;
#{     if (likely(new_size == old_size)) {
#[         return;
#}     }
#[
#[     int len_change = new_size - old_size;
#{     if (likely(len_change > 0)) {
#[         char* new_ptr = rte_pktmbuf_prepend(pd->wrapper, len_change);
#[         if (unlikely(new_ptr == 0)) {
#[             rte_exit(1, "Could not reserve necessary headroom ($${}{%d} additional bytes)", len_change);
#[         }
#[         pd->data = (packet_data_t*)new_ptr;
#[     } else {
#[         char* new_ptr = rte_pktmbuf_adj(pd->wrapper, -len_change);
#[         pd->data = (packet_data_t*)new_ptr;
#}     }
#[     pd->wrapper->pkt_len = new_size;
#} }
#[

#[ // if (some of) the deparsed headers are one after another, this function copies them in one go
#[ void copy_deparse_contents(SHORT_STDPARAMS)
#{ {
#[     uint8_t* dst_start = rte_pktmbuf_mtod(pd->wrapper, uint8_t*);
#[     uint8_t* dst = dst_start;
#{     for (int idx = 0; idx < pd->deparse_hdrinst_count; ) {
#[         header_descriptor_t hdr = pd->headers[pd->header_reorder[idx]];
#[         if (unlikely(hdr.pointer == NULL))    { ++idx; continue; }

#[         uint8_t* copy_start = hdr.pointer;
#[         int copy_start_idx  = idx;
#[         int copy_size       = hdr.size;
#[         int count           = 1;

#[         ++idx;
#{         while (idx < pd->deparse_hdrinst_count) {
#[             header_descriptor_t prevhdr = hdr;
#[             int prevlen = hdr.size;
#[             hdr = pd->headers[pd->header_reorder[idx]];

#[             if (unlikely(hdr.pointer != prevhdr.pointer + prevlen)) break;

#[             ++count;
#[             ++idx;
#[             if (unlikely(hdr.pointer == NULL))    continue;
#[             copy_size += hdr.size;
#}         }

#[         memcpy(dst, copy_start, copy_size);
#[         dst += copy_size;
#}     }
#} }

#{ bool is_packet_dropped(packet_descriptor_t* pd) {
#[      return GET32(src_pkt(pd), EGRESS_META_FLD) == EGRESS_DROP_VALUE;
#} }


#[ void deparse_packet(SHORT_STDPARAMS)
#{ {
#{     if (unlikely(pd->is_deparse_reordering)) {
#{         if (unlikely(is_packet_dropped(pd))) {
#[             return;
#}         }
#[         print_headers(SHORT_STDPARAMS_IN);
#[         store_headers_for_deparse(SHORT_STDPARAMS_IN);
#[         resize_packet_on_deparse(SHORT_STDPARAMS_IN);
#[         copy_deparse_contents(SHORT_STDPARAMS_IN);
#[     } else {
#{         if (unlikely(is_packet_dropped(pd))) {
#[             return;
#[         } else {
#}         }
#}     }
#} }
#[
