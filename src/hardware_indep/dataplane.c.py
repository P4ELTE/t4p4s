# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_declaration, format_statement, format_expr, format_type, gen_format_type, get_method_call_env
from compiler_log_warnings_errors import addError, addWarning
from compiler_common import types, generate_var_name, get_hdrfld_name, unique_everseen

#[ #include "dataplane_impl.h"
#[ #include "dataplane.h"
#[ #include "dataplane_stages.h"

# TODO make this an import from hardware_indep
#[ #include "dpdk_smem.h"

table_infos = [(table, table.short_name + ("/keyless" if table.key_length_bits == 0 else "") + ("/hidden" if table.is_hidden else "")) for table in hlir.tables]

################################################################################
# Table application

for type in unique_everseen([comp['type'] for table in hlir.tables for smem in table.direct_meters + table.direct_counters for comp in smem.components]):
    #[ void apply_direct_smem_$type(register_uint32_t* smem, uint32_t value, char* table_name, char* smem_type_name, char* smem_name) {
    #[    debug("     : applying apply_direct_smem_$type(register_uint32_t (*smem)[1], uint32_t value, char* table_name, char* smem_type_name, char* smem_name)");
    #[ }

################################################################################

#{ void reset_vw_fields(SHORT_STDPARAMS) {
for hdr in hlir.header_instances.filter('urtype.is_metadata', False):
    for fld in hdr.urtype.fields:
        if fld.is_vw:
            #[ pd->headers[HDR(${hdr.name})].var_width_field_bitwidth = 0;
#} }

#{ void reset_headers(SHORT_STDPARAMS) {
for hdr in hlir.header_instances.filter('urtype.is_metadata', False):
    #[     pd->headers[HDR(${hdr.name})].pointer = NULL;

for stk in hlir.header_stacks:
    #[     pd->stacks[STK(${stk.name})].current = -1;

#[     // reset metadatas
#[     memset(pd->headers[HDR(all_metadatas)].pointer, 0, hdr_infos[HDR(all_metadatas)].byte_width * sizeof(uint8_t));
#[
#[     reset_vw_fields(SHORT_STDPARAMS_IN);
#} }


#{ void init_header(header_instance_t hdrinst, const char* hdrname, SHORT_STDPARAMS) {
#{     pd->headers[hdrinst] = (header_descriptor_t) {
#[         .type = hdrinst,
#[         .length = hdr_infos[hdrinst].byte_width,
#[         .pointer = NULL,
#[         .var_width_field_bitwidth = 0,
#[         #ifdef T4P4S_DEBUG
#[             .name = hdrname,
#[         #endif
#}     };
#} }
#[


#{ void init_headers(SHORT_STDPARAMS) {
for hdr in hlir.header_instances.filter('urtype.is_metadata', False):
    #[     init_header(HDR(${hdr.name}), "${hdr.name}", SHORT_STDPARAMS_IN);

#[     // init metadatas
#{     pd->headers[HDR(all_metadatas)] = (header_descriptor_t) {
#[         .type = HDR(all_metadatas),
#[         .length = hdr_infos[HDR(all_metadatas)].byte_width,
#[         .pointer = rte_malloc("all_metadatas_t", hdr_infos[HDR(all_metadatas)].byte_width * sizeof(uint8_t), 0),
#[         .var_width_field_bitwidth = 0,
#}     };
#} }

################################################################################

#{ void init_dataplane(SHORT_STDPARAMS) {
#[     init_headers(SHORT_STDPARAMS_IN);
#[     reset_headers(SHORT_STDPARAMS_IN);

#[     uint32_t res32;
#[     MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), EGRESS_META_FLD, EGRESS_INIT_VALUE);
#} }

################################################################################
# Pipeline

for ctl in hlir.controls:
    if len(ctl.body.components) == 0:
        #[ // skipping method generation for empty control ${ctl.name}
        continue

    #{ void control_${ctl.name}(STDPARAMS)  {
    #[     debug("Control $$[control]{ctl.name}...\n");
    #[     control_locals_${ctl.name}_t local_vars_struct;
    #[     pd->control_locals = (void*)&local_vars_struct;
    for idx, comp in enumerate(ctl.body.components):
        #[     control_stage_${ctl.name}_${idx}(&local_vars_struct, STDPARAMS_IN);
    #} }
    #[

#[ void process_packet(STDPARAMS)
#{ {
for idx, ctl in enumerate(hlir.controls):
    if len(ctl.body.components) == 0:
        #[     // skipping empty control ${ctl.name}
    else:
        #[     control_${ctl.name}(STDPARAMS_IN);

    if hlir.news.model == 'V1Switch' and idx == 1:
        #[     transfer_to_egress(pd);
    if ctl.name == 'egress':
        #[     // TODO temporarily disabled
        #[     // update_packet(STDPARAMS_IN); // we need to update the packet prior to calculating the new checksum
#} }

################################################################################

longest_hdr_name_len = max({len(h.name) for h in hlir.header_instances if not h.urtype.is_metadata})

pkt_name_indent = " " * longest_hdr_name_len

#{ void print_headers(STDPARAMS) {
#{     #ifdef T4P4S_DEBUG
#[         int skips = 0;
#{         for (int i = 0; i < pd->emit_hdrinst_count; ++i) {
#[             header_descriptor_t hdr = pd->headers[pd->header_reorder[i]];
#[             if (unlikely(hdr.pointer == NULL))    ++skips;
#}         }
#[         int emits = pd->emit_hdrinst_count - skips;
#[         debug("   :: Preparing $${}{%d} header%s for storage, skipping " T4LIT(%d) " header%s...\n",
#[               emits, emits != 1 ? "s" : "", skips, skips != 1 ? "s" : "");
#}     #endif
#} }
#[

#{ void store_headers_for_emit(STDPARAMS) {
#[     pd->emit_headers_length = 0;
#{     for (int i = 0; i < pd->emit_hdrinst_count; ++i) {
#[         header_descriptor_t* hdr = &(pd->headers[pd->header_reorder[i]]);
#[
#{         if (unlikely(hdr->pointer == NULL)) {
#{             #ifdef T4P4S_DEBUG
#{                 if (hdr->was_enabled_at_initial_parse) {
#[                     debug("        : -" T4LIT(#%02d ,status) "$$[status][%]{longest_hdr_name_len+1}{s}$$[status]{}{/%02dB} (invalidated)\n", pd->header_reorder[i] + 1, hdr->name, hdr->length);
#}                 }
#}             #endif
#[             continue;
#}         }
#[
#{         if (likely(hdr->was_enabled_at_initial_parse)) {
#[             dbg_bytes(hdr->pointer, hdr->length, "        :  " T4LIT(#%02d) " $$[header][%]{longest_hdr_name_len}{s}/$${}{%02dB} = %s", pd->header_reorder[i] + 1, hdr->name, hdr->length, hdr->pointer == NULL ? T4LIT((invalid),warning) " " : "");
#[             memcpy(pd->header_tmp_storage + hdr_infos[hdr->type].byte_offset, hdr->pointer, hdr->length);
#[         } else {
#[             dbg_bytes(hdr->pointer, hdr->length, "        : +" T4LIT(#%02d) " $$[header][%]{longest_hdr_name_len}{s}/$${}{%02dB} = ", pd->header_reorder[i] + 1, hdr->name, hdr->length);
#}         }
#[
#[         pd->emit_headers_length += hdr->length;
#}     }
#} }
#[

#[ void resize_packet_on_emit(STDPARAMS)
#{ {
#[     int old_length = packet_length(pd);
#[     int new_length = pd->emit_headers_length + pd->payload_length;
#{     if (likely(new_length == old_length)) {
#[         debug(" " T4LIT(::::,status) " Skipping packet resizing: no change in total packet header length\n");
#[         return;
#}     }
#[
#[     int len_change = new_length - old_length;
#{     if (likely(len_change > 0)) {
#[         debug("   " T4LIT(::,status) " Adding   $${}{%02d} bytes %${longest_hdr_name_len}{s}, header length: $${}{%dB} to $${}{%dB}\n", len_change, "to packet", old_length - pd->payload_length, pd->emit_headers_length);
#[         char* new_ptr = rte_pktmbuf_prepend(pd->wrapper, len_change);
#[         if (unlikely(new_ptr == 0)) {
#[             rte_exit(1, "Could not reserve necessary headroom ($${}{%d} additional bytes)", len_change);
#[         }
#[         pd->data = (packet_data_t*)new_ptr;
#[     } else {
#[         debug("   " T4LIT(::,status) " Removing $${}{%02d} bytes %${longest_hdr_name_len}{s}, header length: $${}{%dB} to $${}{%dB}\n", -len_change, "from packet", old_length - pd->payload_length, pd->emit_headers_length);
#[         char* new_ptr = rte_pktmbuf_adj(pd->wrapper, -len_change);
#[         pd->data = (packet_data_t*)new_ptr;
#}     }
#[     pd->wrapper->pkt_len = new_length;
#} }
#[

#[ // if (some of) the emitted headers are one after another, this function copies them in one go
#[ void copy_emit_contents(STDPARAMS)
#{ {
#[     // debug("   :: Putting together packet\n");
#[     uint8_t* dst_start = rte_pktmbuf_mtod(pd->wrapper, uint8_t*);
#[     uint8_t* dst = dst_start;
#{     for (int idx = 0; idx < pd->emit_hdrinst_count; ) {
#[         #ifdef T4P4S_DEBUG
#[             char header_names_txt[1024];
#[             char* header_names_ptr = header_names_txt;
#[         #endif
#[         header_descriptor_t hdr = pd->headers[pd->header_reorder[idx]];
#[         if (unlikely(hdr.pointer == NULL))    { ++idx; continue; }
#[         uint8_t* copy_start     = hdr.pointer;
#[         int copy_start_idx      = idx;
#[         int copy_length         = hdr.length;
#[         int count               = 1;
#[         #ifdef T4P4S_DEBUG
#[             header_names_ptr += sprintf(header_names_ptr, T4LIT(%s,header) "/" T4LIT(%dB), hdr.name, hdr.length);
#[         #endif
#[         ++idx;
#{         while (idx < pd->emit_hdrinst_count && pd->headers[pd->header_reorder[idx]].pointer == hdr.pointer + hdr.length) {
#[             ++count;
#[             hdr = pd->headers[pd->header_reorder[idx]];
#[             ++idx;
#[             if (unlikely(hdr.pointer == NULL))    continue;
#[             copy_length += hdr.length;
#[             #ifdef T4P4S_DEBUG
#[                 header_names_ptr += sprintf(header_names_ptr, " " T4LIT(%s,header) "/" T4LIT(%dB), hdr.name, hdr.length);
#[             #endif
#}         }
#[         // dbg_bytes(copy_start, copy_length, "    : Copying " T4LIT(%d) " %s to byte " T4LIT(#%2ld) " of egress header %s: ", count, count == 1 ? "header" : "adjacent headers", dst - dst_start, header_names_txt);
#[         memcpy(dst, copy_start, copy_length);
#[         dst += copy_length;
#}     }
#} }

#{ bool is_packet_dropped(STDPARAMS) {
#[      return GET_INT32_AUTO_PACKET(pd, HDR(all_metadatas), EGRESS_META_FLD) == EGRESS_DROP_VALUE;
#} }


#[ void emit_packet(STDPARAMS)
#{ {
#{     if (unlikely(pd->is_emit_reordering)) {
#{         if (unlikely(is_packet_dropped(STDPARAMS_IN))) {
#[             debug(" " T4LIT(XXXX,status) " Skipping pre-emit processing: packet is " T4LIT(dropped,status) "\n");
#[             return;
#}         }
#[         debug(" :::: Pre-emit reordering\n");
#[         print_headers(STDPARAMS_IN);
#[         store_headers_for_emit(STDPARAMS_IN);
#[         resize_packet_on_emit(STDPARAMS_IN);
#[         copy_emit_contents(STDPARAMS_IN);
#[     } else {
#{         if (unlikely(is_packet_dropped(STDPARAMS_IN))) {
#[             debug(" " T4LIT(XXXX,status) " Skipping pre-emit processing: packet is " T4LIT(dropped,status) "\n");
#[             return;
#[         } else {
#[             debug(" " T4LIT(::::,status) " Skipping pre-emit processing: no change in packet header structure\n");
#}         }
#}     }
#} }

#[ void handle_packet(uint32_t portid, int pkt_idx, STDPARAMS)
#{ {
#[     int value32;
#[     int res32;
#[
#[     reset_headers(SHORT_STDPARAMS_IN);
#[     set_handle_packet_metadata(pd, portid);
#[
#[     dbg_bytes(pd->data, packet_length(pd), "Handling packet " T4LIT(#%03d) " (port " T4LIT(%d,port) ", $${}{%02dB}): ", pkt_idx, extract_ingress_port(pd), packet_length(pd));
#[
#[     pd->parsed_length = 0;
#[     pd->is_emit_reordering = false;
#[     parse_packet(STDPARAMS_IN);
#[
#[     pd->emit_hdrinst_count = 0;
#[
#[     process_packet(STDPARAMS_IN);
#[
#[     emit_packet(STDPARAMS_IN);
#} }
