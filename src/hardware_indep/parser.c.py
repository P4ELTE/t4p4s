# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from compiler_log_warnings_errors import addError, addWarning
from utils.codegen import format_expr, format_type, format_statement, format_declaration
from compiler_common import statement_buffer_value, generate_var_name


#[ #include "dpdk_lib.h"
#[ #include "util_packet.h"
#[ #include "gen_include.h"

#{ #ifdef T4P4S_STATS
#[     extern t4p4s_stats_t t4p4s_stats_global;
#[     extern t4p4s_stats_t t4p4s_stats_per_packet;
#} #endif

################################################################################

# TODO more than one parser can be present
parser = hlir.parsers[0]

################################################################################

#[ // Returns the sum of all collected variable widths.
#{ int get_var_width_bitwidth(parser_state_t* pstate) {
#[     int retval = 0
for loc in parser.parserLocals:
    if 'is_vw' in loc.urtype and loc.urtype.is_vw:
        #[ + pstate->${loc.name}_var
#[     ;

#[ return retval;

#} }

################################################################################

#{ void drop_packet(STDPARAMS) {
#[     uint32_t value32; (void)value32;
#[     uint32_t res32; (void)res32;
#[     MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), EGRESS_META_FLD, EGRESS_DROP_VALUE);
#} }


#{ void init_parser_state(parser_state_t* pstate) {
for local in parser.parserLocals.filter('node_type', 'Declaration_Instance'):
    #[ ${local.urtype.name}_t_init(pstate->${local.name});
#} }

for s in parser.states:
    if s.is_reachable:
        #[ static void parser_state_${s.name}(STDPARAMS);
    else:
        #[ // state ${s.name} is not reachable

#{ void cannot_parse_hdr(const char* varwidth_txt, const char* hdr_name, uint32_t hdrlen, STDPARAMS) {
#{     #ifdef T4P4S_DEBUG
#{         if (pd->parsed_length == pd->wrapper->pkt_len) {
#[             debug("    " T4LIT(!,warning) " Missing %sheader " T4LIT(%s,header) "/" T4LIT(%d) "B @ offset " T4LIT(%d) "\n", varwidth_txt, hdr_name, hdrlen, pd->parsed_length);
#[         } else {
#[             debug("    " T4LIT(!,warning) " Trying to parse %sheader " T4LIT(%s,header) "/" T4LIT(%d) "B at offset " T4LIT(%d) ", " T4LIT(missing %d bytes,warning) "\n",
#[                   varwidth_txt, hdr_name, hdrlen, pd->parsed_length, pd->parsed_length + hdrlen - pd->wrapper->pkt_len);
#}         }
#}     #endif
#} }


for hdr in hlir.header_instances.filterfalse(lambda hdr: hdr.urtype.is_metadata):
    #{ static int parser_extract_${hdr.name}(uint32_t vwlen, STDPARAMS) {
    #[     uint32_t value32; (void)value32;
    #[     uint32_t res32; (void)res32;
    #[     parser_state_t* local_vars = pstate;

    hdrtype = hdr.urtype

    is_vw = hdrtype.is_vw

    #[     uint32_t hdrlen = (${hdr.urtype.size} + vwlen) / 8;
    #{     if (unlikely(pd->parsed_length + hdrlen > pd->wrapper->pkt_len)) {
    #[         cannot_parse_hdr("${"variable width " if is_vw else ""}", "${hdr.name}", hdrlen, STDPARAMS_IN);
    #[         return -1; // parsed after end of packet
    #}     }

    #[     header_descriptor_t* hdr = &(pd->headers[HDR(${hdr.name})]);

    #[     hdr->pointer = pd->extract_ptr;
    #[     hdr->was_enabled_at_initial_parse = true;
    #[     hdr->length = hdrlen;
    #[     hdr->var_width_field_bitwidth = vwlen;

    for fld in hdrtype.fields:
        if fld.preparsed and fld.size <= 32:
            #[     EXTRACT_INT32_AUTO_PACKET(pd, HDR(${hdr.name}), FLD(${hdr.name},${fld.name}), value32)
            #[     pd->fields.FLD(${hdr.name},${fld.name}) = value32;
            #[     pd->fields.ATTRFLD(${hdr.name},${fld.name}) = NOT_MODIFIED;

    #[     dbg_bytes(hdr->pointer, hdr->length,
    #[               "   :: Parsed ${"variable width " if is_vw else ""}header" T4LIT(#%d) " $$[header]{hdr.name}/$${}{%dB}: ", hdr_infos[HDR(${hdr.name})].idx, hdr->length);

    #[     pd->parsed_length += hdrlen;
    #[     pd->extract_ptr += hdrlen;

    #[     return hdrlen;

    #} }
    #[


# TODO find a less convoluted way to get to the header
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
                addError("Finding header", f"There is no single header that corresponds to {hdrtype.name}")
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

    if s.name in ('accept', 'reject'):
        continue

    if s.selectExpression.node_type != 'SelectExpression':
        continue

    #{ static void parser_state_${s.name}_next_state(STDPARAMS) {
    #[     parser_state_t parameters = *pstate;
    bexpr = format_expr(s.selectExpression)
    prebuf, postbuf = statement_buffer_value()
    #[     $prebuf
    #=     bexpr
    #[     $postbuf
    #} }
    #[


def state_component_name(s, idx, component):
    def methodcall_info(mc):
        m = mc.method
        if 'expr' not in m:
            return f'_{m.path.name}'
        hdrname = m.expr.member if 'member' in m.expr else m.expr.path.name
        method_name = m.member
        return f'_{hdrname}_{method_name}'

    def member_info(m):
        hdrname = m.expr.member if 'member' in m.expr else m.expr.path.name
        return f'_{hdrname}${m.member}'

    info = ''
    if 'call' not in component:
        info = f'_{component.node_type}'
        if component.node_type == 'AssignmentStatement':
            left = ''
            if (m := component.left).node_type == 'Member':
                left = member_info(m)
            if (pe := component.left).node_type == 'PathExpression':
                left = f'_{pe.path.name}'

            right = ''
            if (mc := component.right).node_type == 'MethodCallExpression':
                right = methodcall_info(mc)
            elif (const := component.right).node_type == 'Constant':
                right = f'_const_{const.value}'
            elif (pe := component.right).node_type == 'PathExpression':
                right = f'_assign_{pe.path.name}'

            info = f'{left}{right}'
        if component.node_type == 'MethodCallStatement':
            info = methodcall_info(component.methodCall)
    elif component.call == 'extract_header':
        is_underscore_header, hdr, hdrt = component_extract_info(component)

        if is_underscore_header:
            info = f'_extract_{hdrt.name}_underscore'
        else:
            info = f'_extract_{hdr.name}'

    return f'parser_state_{s.name}_{idx:03}{info}'


def component_extract_info(component):
    arg0 = component.methodCall.arguments['Argument'][0]
    hdr = get_hdrinst(arg0, component)

    # note: a hack (an attribute's proper value should be the determinant, not the presence/absence of an attribute),
    #       but it looks like the best way to determine whether we are extracting to the underscore identifier
    # is_underscore_header = hdr is None or not hasattr(hdr, 'annotations')
    is_underscore_header = False

    if is_underscore_header:
        hdrt = arg0.expression.hdr_ref.urtype
        size = (hdrt.size+7)//8
    else:
        hdrt = hdr.urtype

    return is_underscore_header, hdr, hdrt


for s in parser.states:
    if not s.is_reachable:
        continue

    for idx, component in enumerate(s.components):
        #{ static void ${state_component_name(s, idx, component)}(STDPARAMS) {
        #[     uint32_t res32; (void)res32;
        #[     parser_state_t* local_vars = pstate;

        if 'call' in component:
            if component.call != 'extract_header':
                addWarning('invoking state component', f'Unknown state component call of type {component.call}')
                continue

            is_underscore_header, hdr, hdrt = component_extract_info(component)

            # TODO remove?
            args = component.methodCall.arguments
            var = generate_var_name('vwlen')
            if len(args) == 1:
                #[     int $var = 0;
            else:
                bexpr = format_expr(args[1].expression)
                prebuf, postbuf = statement_buffer_value()
                #[     $prebuf
                #[     int $var = ${bexpr};
                #[     $postbuf

            #[     int offset_${hdr.name} = parser_extract_${hdr.name}($var, STDPARAMS_IN);
            #{     if (unlikely(offset_${hdr.name}) < 0) {
            #[         drop_packet(STDPARAMS_IN);
            #[         debug("   " T4LIT(XX,status) " " T4LIT(Dropping packet,status) "\n");
            #[         return;
            #}     }
        else:
            #[     ${format_statement(component, parser)}
        #} }
        #[ 


for s in parser.states:
    if not s.is_reachable:
        continue

    #{ static void parser_state_${s.name}(STDPARAMS) {
    #{     #ifdef T4P4S_STATS
    #[         t4p4s_stats_global.parser_state__${s.name} = true;
    #[         t4p4s_stats_per_packet.parser_state__${s.name} = true;
    #}     #endif

    if s.name == 'accept':
        #[     pd->payload_length = packet_length(pd) - (pd->extract_ptr - (void*)pd->data);

        #{     if (pd->payload_length > 0) {
        #[         dbg_bytes(pd->data + pd->parsed_length, pd->payload_length, " " T4LIT(%%%%%%%%,success) " Packet is $$[success]{}{accepted}, " T4LIT(%d) "B of headers, " T4LIT(%d) "B of payload: ", pd->parsed_length, pd->payload_length);
        #[     } else {
        #[         debug(" " T4LIT(%%%%%%%%,success) " Packet is $$[success]{}{accepted}, " T4LIT(%d) "B of headers, " T4LIT(empty payload) "\n", pd->parsed_length);
        #}     }
    elif s.name == 'reject':
        #[     debug(" " T4LIT(XXXX,status) " Parser state $$[parserstate]{s.name}, packet is $$[status]{}{dropped}\n");
        #[     drop_packet(STDPARAMS_IN);
    else:
        #[     debug(" %%%%%%%% Parser state $$[parserstate]{s.name}\n");

        for idx, component in enumerate(s.components):
            #[     ${state_component_name(s, idx, component)}(STDPARAMS_IN);

        if s.selectExpression.node_type == 'PathExpression':
            #[     parser_state_${s.selectExpression.path.name}(STDPARAMS_IN);
        else:
            #[     parser_state_${s.name}_next_state(STDPARAMS_IN);

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
    #[     "${hdr.name}",
#} };

#{ const char* field_names[FIELD_COUNT] = {
for hdr in hlir.header_instances:
    #[     // ${hdr.name}
    for fld in hdr.urtype.fields:
        #[         "${fld.name}", // ${hdr.name}.${fld.name}
#} };
