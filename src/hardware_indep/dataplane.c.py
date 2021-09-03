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

# TODO make this an import from hardware_indep
#[ #include "dpdk_smem.h"

table_infos = [(table, table.short_name + ("/keyless" if table.key_bit_size == 0 else "") + ("/hidden" if table.is_hidden else "")) for table in hlir.tables]

################################################################################
# Table application

for type in unique_everseen([comp['type'] for table in hlir.tables for smem in table.direct_meters + table.direct_counters for comp in smem.components]):
    #[ void apply_direct_smem_$type(register_uint32_t* smem, uint32_t value, char* table_name, char* smem_type_name, char* smem_name) {
    #[    debug("     : applying apply_direct_smem_$type(register_uint32_t (*smem)[1], uint32_t value, char* table_name, char* smem_type_name, char* smem_name)");
    #[ }

################################################################################

#{ void reset_vw_fields(SHORT_STDPARAMS) {
for hdr in hlir.header_instances.filterfalse('urtype.is_metadata').filter('urtype.is_vw'):
    #[ pd->headers[HDR(${hdr.name})].vw_size = 0;
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


#{ void init_header(header_instance_e hdrinst, const char* hdrname, SHORT_STDPARAMS) {
#{     pd->headers[hdrinst] = (header_descriptor_t) {
#[         .type = hdrinst,
#[         .size = hdr_infos[hdrinst].byte_width,
#[         .pointer = NULL,
#[         .vw_size = 0,
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
#[         .size = hdr_infos[HDR(all_metadatas)].byte_width * 8,
#[         .pointer = rte_malloc("all_metadatas_t", hdr_infos[HDR(all_metadatas)].byte_width * sizeof(uint8_t), 0),
#[         .vw_size = 0,
#}     };
#} }

################################################################################

#{ void init_dataplane(SHORT_STDPARAMS) {
#[     init_headers(SHORT_STDPARAMS_IN);
#[     reset_headers(SHORT_STDPARAMS_IN);

#[     MODIFY(dst_pkt(pd), EGRESS_META_FLD, src_32(EGRESS_INIT_VALUE), ENDIAN_KEEP);
#} }

################################################################################
# Pipeline

for ctl in hlir.controls:
    if len(ctl.body.components) == 0:
        #[ // skipping method generation for empty control ${ctl.name}
        continue

    #{ void control_${ctl.name}(STDPARAMS)  {
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

#[ extern void deparse_packet(STDPARAMS);
#[

#[ void handle_packet(uint32_t portid, int pkt_idx, STDPARAMS)
#{ {
#[     reset_headers(SHORT_STDPARAMS_IN);
#[     set_handle_packet_metadata(pd, portid);
#[
#[     dbg_bytes(pd->data, packet_size(pd), "Handling packet " T4LIT(#%03d) " (port " T4LIT(%d,port) ", $${}{%02dB}): ", pkt_idx, get_ingress_port(pd), packet_size(pd));
#[
#[     pd->parsed_size = 0;
#[     pd->is_deparse_reordering = false;
#[     parse_packet(STDPARAMS_IN);
#[
#[     pd->deparse_hdrinst_count = 0;
#[
#[     process_packet(STDPARAMS_IN);
#[
#[     deparse_packet(STDPARAMS_IN);
#} }
#[
