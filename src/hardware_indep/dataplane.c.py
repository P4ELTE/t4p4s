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

for parser in hlir.parsers:
    #[ void parser_state_${parser.name}_start(STDPARAMS);

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


#{ void init_metadata_header(SHORT_STDPARAMS) {
#{     pd->headers[HDR(all_metadatas)] = (header_descriptor_t) {
#[         .type = HDR(all_metadatas),
#[         .size = hdr_infos[HDR(all_metadatas)].byte_width * 8,
#[         .pointer = rte_malloc("all_metadatas_t", hdr_infos[HDR(all_metadatas)].byte_width * sizeof(uint8_t), 0),
#[         .vw_size = 0,
#}     };
#} }
#[


#{ void init_headers(SHORT_STDPARAMS) {
for hdr in hlir.header_instances.filter('urtype.is_metadata', False):
    #[     init_header(HDR(${hdr.name}), "${hdr.name}", SHORT_STDPARAMS_IN);

#[     init_metadata_header(SHORT_STDPARAMS_IN);
#} }
#[

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

################################################################################

def has_annotation(node, annot_name):
    return node.annotations.get(annot_name) is not None

def gen_use_package(decl, depth):
    for arg in decl.arguments.map('expression'):
        if arg.node_type == 'PathExpression':
            decl2 = hlir.decl_instances.get(arg.path.name)
            #[ // ${arg.urtype.name} ${arg.path.name}
            #{ {
            #= gen_use_package(decl2, depth+1)
            #} }
        elif (ctl := arg.urtype).node_type == 'Type_Control':
            if len(hlir.controls.get(ctl.name).body.components) == 0:
                #[ // control ${ctl.name} is empty
                continue
            #[ control_${ctl.name}(STDPARAMS_IN);
            if ctl.name in hlir.news.deparsers or has_annotation(ctl, 'deparser'):
                #[     deparse_packet(SHORT_STDPARAMS_IN);
        elif (parser := arg.urtype).node_type == 'Type_Parser':
            #[ pd->is_deparse_reordering = false;
            #[ parser_state_${parser.name}_start(STDPARAMS_IN);
        elif (extern := arg.urtype).node_type == 'Type_Extern':
            #[ // nonpkg ${arg.urtype.name} ${arg.urtype.node_type}
        else:
            addWarning('Unknown item in package', f'Item {arg.name} in package {pkg.urtype.name} {pkg.name} is of unknown type {arg.urtype.name}')

main_decl = hlir.decl_instances.filter('urtype.name', hlir.news.model)[0]
#{ void process_packet(STDPARAMS) {
#= gen_use_package(main_decl, 0)
#} }
#[

#[ extern void deparse_packet(SHORT_STDPARAMS);
#[

#[ void handle_packet(uint32_t portid, int pkt_idx, STDPARAMS)
#{ {
#[     reset_headers(SHORT_STDPARAMS_IN);
#[     set_handle_packet_metadata(pd, portid);
#[
#[     dbg_bytes(pd->data, packet_size(pd), "Handling packet " T4LIT(#%03d) " (port " T4LIT(%d,port) ", $${}{%02dB}): ", pkt_idx, get_ingress_port(pd), packet_size(pd));
#[
#[     pd->parsed_size = 0;
#[     pd->extract_ptr = pd->data;
#[
#[     pd->deparse_hdrinst_count = 0;
#[
#[     process_packet(STDPARAMS_IN);
#[
#[     deparse_packet(SHORT_STDPARAMS_IN);
#} }
#[
