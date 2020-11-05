# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from compiler_log_warnings_errors import addError, addWarning
from utils.codegen import format_expr, format_statement, format_declaration
from compiler_common import statement_buffer_value


#[ #include "dpdk_lib.h"
#[ #include "util_packet.h"
#[ #include "gen_include.h"

#{ #ifdef T4P4S_STATS
#[     extern t4p4s_stats_t t4p4s_stats;
#} #endif

################################################################################

#{ void drop_packet(STDPARAMS) {
#[     uint32_t value32; (void)value32;
#[     uint32_t res32; (void)res32;
#[     MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), EGRESS_META_FLD, EGRESS_DROP_VALUE);
#[     debug("    " T4LIT(:,status) " " T4LIT(Dropping package,status) "\n");
#} }


# TODO more than one parser can be present
parser = hlir.parsers[0]


#{ void init_parser_state(parser_state_t* pstate) {
for local in parser.parserLocals.filter('node_type', 'Declaration_Instance'):
    #[ ${local.urtype.name}_t_init(pstate->${local.name});
#} }

for s in parser.states:
    if s.is_reachable:
        #[ static void parser_state_${s.name}(STDPARAMS);
    else:
        #[ // state ${s.name} is not reachable

for hdr in hlir.header_instances.filterfalse(lambda hdr: hdr.urtype.is_metadata):
    #{ static int parser_extract_${hdr.name}(STDPARAMS) {
    #[     uint32_t value32; (void)value32;
    #[     uint32_t res32; (void)res32;
    #[     parser_state_t* local_vars = pstate;

    hdrtype = hdr.urtype

    is_vw = hdrtype.is_vw

    # TODO properly implement varwidth

    #[ uint32_t vwlen = 0;
    #[ uint32_t hdrlen = (${hdr.urtype.size} + vwlen) / 8;
    #{ if (unlikely(pd->parsed_length + hdrlen > pd->wrapper->pkt_len)) {
    #{     #ifdef T4P4S_DEBUG
    #{         if (pd->parsed_length == pd->wrapper->pkt_len) {
    #[             debug("    " T4LIT(!,warning) " Missing ${"variable width " if is_vw else ""}header $$[header]{hdr.name}/" T4LIT(%d) "B @ offset " T4LIT(%d) "\n", hdrlen, pd->parsed_length);
    #[         } else {
    #[             debug("    " T4LIT(!,warning) " Trying to parse ${"variable width " if is_vw else ""}header $$[header]{hdr.name}/" T4LIT(%d) "B at offset " T4LIT(%d) ", " T4LIT(missing %d bytes,warning) "\n", hdrlen, pd->parsed_length, pd->parsed_length + hdrlen - pd->wrapper->pkt_len);
    #}         }
    #}     #endif
    #[
    #[     return -1; // parsed after end of packet
    #} }

    #[ header_descriptor_t* hdr = &(pd->headers[HDR(${hdr.name})]);

    #[ hdr->pointer = pd->extract_ptr;
    #[ hdr->was_enabled_at_initial_parse = true;
    #[ hdr->length = hdrlen;
    #[ hdr->var_width_field_bitwidth = vwlen;

    for fld in hdrtype.fields:
        if fld.preparsed and fld.size <= 32:
            #[ EXTRACT_INT32_AUTO_PACKET(pd, HDR(${hdr.name}), FLD(${hdr.name},${fld.name}), value32)
            #[ pd->fields.FLD(${hdr.name},${fld.name}) = value32;
            #[ pd->fields.ATTRFLD(${hdr.name},${fld.name}) = NOT_MODIFIED;

    #[     dbg_bytes(hdr->pointer, hdr->length,
    #[               "   :: Extracted ${"variable width " if is_vw else ""}header instance " T4LIT(#%d) " $$[header]{hdr.name}/$${}{%dB}: ", hdr_infos[HDR(${hdr.name})].idx, hdr->length);

    #[     pd->parsed_length += hdrlen;
    #[     pd->extract_ptr += hdrlen;

    #[     return hdrlen;

    #} }


# TODO find a less convoluted way to get to the header instance
def get_hdrinst(arg0, component):
    if 'path' in arg0:
        hdrinst_name = arg0.path.name

        hdrtype = component.header.urtype
        dvar = parser.parserLocals.get(hdrinst_name, 'Declaration_Variable')
        if dvar:
            insts = [hi for hi in hlir.header_instances['StructField'] if hi.urtype.name == hdrtype.name]
            if len(insts) == 1:
                hdrinst = insts[0]
            elif len(insts) == 0:
                # note: it is defined as a local variable
                return dvar.urtype
            else:
                addError("Finding header instance", f"There is no single header instance that corresponds to {hdrtype.name}")
        else:
            return hlir.header_instances.get(hdrinst_name, 'Declaration_Variable', lambda hi: hi.urtype.name == hdrtype.name)
    elif 'hdr_ref' in (mexpr := component.methodCall.method.expr):
        hdrinst = mexpr.hdr_ref
        return hdrinst
    else:
        a0e = arg0.expression
        if "member" in a0e:
            hdrtype = component.header.urtype
            hdrinst_name = a0e.member
            hdrinst = hlir.header_instances.get(hdrinst_name, 'StructField', lambda hi: hi.urtype.name == hdrtype.name)
        else:
            hdrinst_name = a0e.hdr_ref.name
            hdrinst = hlir.header_instances.get(hdrinst_name)
        return hdrinst


for s in parser.states:
    if not s.is_reachable:
        continue

    #{ static void parser_state_${s.name}(STDPARAMS) {

    #{ #ifdef T4P4S_STATS
    #[     t4p4s_stats.parser_state__${s.name} = true;
    #} #endif

    #[     uint32_t value32; (void)value32;
    #[     uint32_t res32; (void)res32;
    #[     parser_state_t* local_vars = pstate;
    #[     debug(" :::: Parser state $$[parserstate]{s.name}\n");

    for component in s.components:
        if 'call' not in component:
            #[ ${format_statement(component, parser)}
            continue

        if component.call != 'extract_header':
            continue

        arg0 = component.methodCall.arguments['Argument'][0]
        hdr = get_hdrinst(arg0, component)
        if hdr is None:
            hdrt = arg0.expression.hdr_ref.urtype
            if hdrt.size % 8 != 0:
                addWarning('extracting underscore header', f'Extracting non-byte-aligned header type {hdrt.name}/{hdrt.size}b as noname header')
            size = (hdrt.size+7)//8
            #[ // extracting to underscore argument, $size bytes (${hdrt.size} bits)
            #[ debug("   :: Extracting " T4LIT(${hdrt.name},header) "/" T4LIT($size) " as " T4LIT(noname header,header) "\n");
            #[ pd->extract_ptr += $size;
            #[ pd->is_emit_reordering = true; // a noname header cannot be emitted
        else:
            #[ int offset_${hdr.name} = parser_extract_${hdr.name}(STDPARAMS_IN);
            #{ if (unlikely(offset_${hdr.name}) < 0) {
            #[     drop_packet(STDPARAMS_IN);
            #[     return;
            #} }
        #[ 


    if 'selectExpression' not in s:
        if s.name == 'accept':
            #[ debug("   " T4LIT(::,success) " Packet is $$[success]{}{accepted}, total length of headers: " T4LIT(%d) " bytes\n", pd->parsed_length);

            #[ pd->payload_length = packet_length(pd) - (pd->extract_ptr - (void*)pd->data);

            #{ if (pd->payload_length > 0) {
            #[     dbg_bytes(pd->data + pd->parsed_length, pd->payload_length, "    : " T4LIT(Payload,header) " is $${}{%d} bytes: ", pd->payload_length);
            #[ } else {
            #[     debug("    " T4LIT(:,status) " " T4LIT(Payload,header) " is " T4LIT(empty,status) "\n");
            #} }
        elif s.name == 'reject':
            #[ debug("   " T4LIT(::,status) " Packet is $$[status]{}{dropped}\n");
            #[ drop_packet(STDPARAMS_IN);
    else:
        b = s.selectExpression

        if b.node_type == 'PathExpression':
            #[ parser_state_${b.path.name}(STDPARAMS_IN);
        elif b.node_type == 'SelectExpression':
            bexpr = format_expr(b)
            prebuf, postbuf = statement_buffer_value()
            #[ $prebuf
            #= bexpr
            #[ $postbuf
    #} }
    #[

#[ void parse_packet(STDPARAMS) {
#[     pd->parsed_length = 0;
#[     pd->extract_ptr = pd->data;
#[     parser_state_start(STDPARAMS_IN);
#[ }
#[


#{ const char* header_instance_names[HEADER_COUNT] = {
for hdr in hlir.header_instances:
    #[ "${hdr.name}",
#} };

#{ const char* field_names[FIELD_COUNT] = {
for hdr in hlir.header_instances:
    for fld in hdr.urtype.fields:
        #[ "${fld.name}", // in header ${hdr.name}
#} };


#[ // Returns the sum of all collected variable widths.
#{ int get_var_width_bitwidth(parser_state_t* pstate) {
#[     int retval = 0
for loc in parser.parserLocals:
    if 'is_vw' in loc.urtype and loc.urtype.is_vw:
        #[ + pstate->${loc.name}_var
#[     ;

#[ return retval;

#} }
