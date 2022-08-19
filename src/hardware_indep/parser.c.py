# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from compiler_log_warnings_errors import addError, addWarning
from utils.codegen import format_expr, format_type, format_statement, format_declaration
from compiler_common import statement_buffer_value, generate_var_name, get_hdr_name, unique_everseen

import functools


#[ #include "dpdk_lib.h"
#[ #include "util_packet.h"
#[ #include "gen_include.h"
#[ #include "parser_stages.h"

#{ #ifdef T4P4S_STATS
#[     extern t4p4s_stats_t t4p4s_stats_global;
#[     extern t4p4s_stats_t t4p4s_stats_per_packet;
#} #endif

################################################################################

#{ void gen_parse_drop_msg(int apparent_hdr_len, const char* hdrname, int max_stkhdr_count) {
#{     if (apparent_hdr_len == PARSED_AFTER_END_OF_PACKET) {
#[         debug("   " T4LIT(XX,status) " " T4LIT(Dropping packet,status) ": tried to parse " T4LIT(%s,header) " but it overshot the end of the packet\n", hdrname);
#[     } else if (apparent_hdr_len == PARSED_OVER_STACK_SIZE) {
#[         debug("   " T4LIT(XX,status) " " T4LIT(Dropping packet,status) ": cannot have more than " T4LIT(%d) " headers in stack " T4LIT(%s,header) "\n", max_stkhdr_count, hdrname);
#[     } else {
#[         debug("   " T4LIT(XX,status) " " T4LIT(Dropping packet,status) ": its length is negative (%d)\n", apparent_hdr_len);
#}     }
#} }
#[


#{ void drop_packet(STDPARAMS) {
#[     MODIFY(dst_pkt(pd), EGRESS_META_FLD, src_32(EGRESS_DROP_VALUE), ENDIAN_KEEP);
#} }
#[

#{ #ifdef T4P4S_DEBUG
#[     char transition_cond[1024];
#[
#{     void set_transition_txt(const char* transition_txt) {
#[         strcpy(transition_cond, transition_txt);
#}     }
#[ #else
#{     void set_transition_txt(const char* transition_txt) {
#[         // do nothing
#}     }
#} #endif
#[

#{ void check_hdr_valid(packet_descriptor_t* pd, field_instance_e fld, const char* unspec) {
#{     #ifdef T4P4S_DEBUG
#[         header_instance_e hdr = fld_infos[fld].header_instance;
#{         if (unlikely(!is_header_valid(hdr, pd))) {
#[             const char* hdrname = hdr_infos[hdr].name;
#[             const char* fldname = field_names[fld];
#[             debug("   " T4LIT(!!,warning) " Access to field in invalid header " T4LIT(%s,warning) "." T4LIT(%s,field) ", returning \"unspecified\" value " T4LIT(%s) "\n", hdrname, fldname, unspec);
#}         }
#}     #endif
#} }


#{ void init_parser_state(parser_state_t* pstate) {
#[     set_transition_txt("");
for parser in hlir.parsers:
    for local in parser.parserLocals.filter('node_type', 'Declaration_Instance'):
        #[     ${local.urtype.name}_t_init(pstate->${local.name});
#} }
#[

#{ void cannot_parse_hdr(const char* varwidth_txt, const char* hdr_name, int hdrlen, int vwlen, STDPARAMS) {
#{     #ifdef T4P4S_DEBUG
#[         int total_bytes = (hdrlen + vwlen) / 8;
#{         if (pd->parsed_size == pd->wrapper->pkt_len) {
#[             debug("    " T4LIT(!,warning) " Missing %sheader " T4LIT(%s,header) "/" T4LIT(%d) "+" T4LIT(%d) "B at offset " T4LIT(%d) "\n",
#[                   varwidth_txt, hdr_name, (hdrlen+7) / 8, (vwlen+7) / 8, pd->parsed_size);
#[         } else {
#[             debug("    " T4LIT(!,warning) " Trying to parse %sheader " T4LIT(%s,header) "/" T4LIT(%d) "+" T4LIT(%d) "B at offset " T4LIT(%d) ", " T4LIT(missing %d bytes,warning) "\n",
#[                   varwidth_txt, hdr_name, (hdrlen+7) / 8, (vwlen+7) / 8, pd->parsed_size, pd->parsed_size + total_bytes - pd->wrapper->pkt_len);
#}         }
#}     #endif
#} }
#[

# TODO find a less convoluted way to get to the header
def get_hdrinst(arg0, component):
    if 'path' in arg0:
        hdrinst_name = arg0.path.name

        hdrtype = component.header.urtype
        dvar = parser.parserLocals.get(hdrinst_name, 'Declaration_Variable')
        if not dvar:
            return hlir.header_instances.get(hdrinst_name, 'Declaration_Variable', lambda hi: hi.urtype.name == hdrtype.name)

        insts = [hi for hi in hlir.header_instances['StructField'] if hi.urtype.name == hdrtype.name]
        if len(insts) == 1:
            return insts[0]
        if len(insts) == 0:
            # note: it is defined as a local variable
            return dvar.urtype

        addError("Finding header", f"There is no single header that corresponds to {hdrtype.name}")
        return None

    if 'hdr_ref' in (mexpr := component.methodCall.method.expr):
        return mexpr.hdr_ref

    a0e = arg0.expression

    if a0e.node_type == 'ArrayIndex':
        return hlir.header_instances.get(get_hdr_name(a0e))

    if 'expr' in a0e and a0e.expr.urtype.node_type == 'Type_Stack':
        # TODO do not always return #0
        return hlir.header_instances.get(f'{a0e.expr.member}_0')

    if "member" in a0e:
        hdrtype = component.header.urtype
        hdrinst_name = a0e.member
        return hlir.header_instances.get(hdrinst_name, 'StructField', lambda hi: hi.urtype.name == hdrtype.name)

    hdrinst_name = a0e.hdr_ref.name
    return hlir.header_instances.get(hdrinst_name)


for parser in hlir.parsers:
    for s in parser.states:
        if s.is_reachable:
            #[ void parser_state_${parser.name}_${s.name}(STDPARAMS);
        else:
            #[ // state ${s.name} is not reachable
    #[
#[


#{ void print_missed_transition_conditions(const char*const* texts, int idx) {
#{     #ifdef T4P4S_SHOW_MISSED_TRANSITION_CONDITIONS
#{         for (int i = 0; i < idx; ++i) {
#[             if (!strcmp("", texts[i]))    continue;
#[             debug("   %%%% Transition condition " T4LIT(not met,status) ": %s\n", texts[i]);
#}         }
#}     #endif
#} }
#[

for parser in hlir.parsers:
    for s in parser.states:
        if not s.is_reachable:
            continue

        if s.name in ('accept', 'reject'):
            continue

        # note: this is a direct state transition, complex select code is not needed
        if s.selectExpression.node_type == 'PathExpression':
            continue

        #{ void parser_state_${parser.name}_${s.name}_next_state(STDPARAMS) {
        #[     parser_state_t* local_vars = pstate;
        #[     parser_state_t parameters = *pstate;
        bexpr = format_expr(s.selectExpression)
        prebuf, postbuf = statement_buffer_value()
        #[     $prebuf
        #=     bexpr
        #[     $postbuf
        #} }
        #[


def state_component_name(parser_name, s, idx, component):
    def methodcall_info(mc):
        m = mc.method
        if 'expr' not in m:
            return f'_{m.path.name}'

        if m.expr.node_type == 'ArrayIndex':
            stk_name = m.expr.left.member
            idx = m.expr.right.value
            hdrname = f'{stk_name}__{idx}'
        elif 'member' in m.expr:
            hdrname = m.expr.member
        else:
            hdrname = m.expr.path.name

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

    return f'parser_state_{parser_name}_{s.name}_{idx:03}{info}'


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

for parser in hlir.parsers:
    for s in parser.states:
        if not s.is_reachable:
            continue

        for idx, component in enumerate(s.components):
            #{ bool ${state_component_name(parser.name, s, idx, component)}(STDPARAMS) {
            #[     parser_state_t* local_vars = pstate;

            if 'call' in component:
                if component.call != 'extract_header':
                    addWarning('invoking state component', f'Unknown state component call of type {component.call}')
                    continue

                is_underscore_header, hdr, hdrt = component_extract_info(component)

                args = component.methodCall.arguments
                vwlen_var = generate_var_name('vwlen')
                if len(args) == 1:
                    #[     uint32_t ${vwlen_var} = 0;
                else:
                    vw_size = hdr.urtype.vw_fld.urtype.size
                    vw_size_bytes = (vw_size+7) // 8

                    bexpr = format_expr(args[1].expression)
                    prebuf, postbuf = statement_buffer_value()
                    #[     $prebuf
                    #[     uint32_t ${vwlen_var} = ((${bexpr}) + 7) / 8;
                    #[     $postbuf

                    #{     if (unlikely(${vwlen_var} < 0)) {
                    #[         debug("    " T4LIT(!,error) " Determined variable length for field " T4LIT(${hdr.name},header) "." T4LIT(%s,field) " = " T4LIT(%d) " " T4LIT(is negative,error) "\n", field_names[hdr_infos[HDR(${hdr.name})].var_width_field], ${vwlen_var});

                    #[         drop_packet(STDPARAMS_IN);
                    #[         return false;
                    #[     } else if (unlikely(${vwlen_var} > ${vw_size_bytes})) {
                    #[         debug("    " T4LIT(!,error) " Determined variable length for field " T4LIT(${hdr.name},header) "." T4LIT(%s,field) " = " T4LIT(%d) " " T4LIT(is larger than the maximum varbit size,error) " " T4LIT(%d) "\n", field_names[hdr_infos[HDR(${hdr.name})].var_width_field], ${vwlen_var}, ${vw_size_bytes});

                    #[         drop_packet(STDPARAMS_IN);
                    #[         return false;
                    #{     #ifdef T4P4S_DEBUG
                    #[     } else {
                    #[         debug("    : Determined variable length for field " T4LIT(${hdr.name},header) "." T4LIT(%s,field) " = " T4LIT(%d) "B\n", field_names[hdr_infos[HDR(${hdr.name})].var_width_field], ${vwlen_var} / 8);
                    #}     #endif
                    #}     }

                #[     int ${hdr.name}_len = parser_extract_${hdr.name}(${vwlen_var}, STDPARAMS_IN);
                #{     if (unlikely(${hdr.name}_len < 0)) {
                #[         gen_parse_drop_msg(${hdr.name}_len, "${hdr.name}", -1 /* ignored */);
                #[         drop_packet(STDPARAMS_IN);
                #[         return false;
                #}     }

                # if any header is skipped (extracted as (_)), it will not be emitted during deparsing
                if any(hlir.header_instances.filterfalse(lambda hdr: hdr.urtype.is_metadata).map(lambda hdr: hdr.is_local)):
                    #[     pd->is_deparse_reordering = true;
            else:
                #[     ${format_statement(component, parser)}
            #[     return true;
            #} }
            #[

nonmeta_hdrinst_count = len(hlir.header_instances.filterfalse('urtype.is_metadata'))
#{ int get_active_hdr_count(STDPARAMS) {
#[     int retval = 0;
#{     for (int i = 0; i < ${nonmeta_hdrinst_count}; ++i) {
#[         retval += is_header_valid(i, pd) ? 1 : 0;
#}     }
#[     return retval;
#} }
#[

#{ void print_parsing_success(STDPARAMS) {
#{     #ifdef T4P4S_DEBUG
#[         int hdr_count = get_active_hdr_count(STDPARAMS_IN);
#{         if (pd->payload_size > 0) {
#[             dbg_bytes(pd->data + pd->parsed_size, pd->payload_size, " " T4LIT(%%%%%%%%,success) " Packet is " T4LIT(accepted,success) ", " T4LIT(%d) "B in " T4LIT(%d) " header%s, " T4LIT(%d) "B of payload: ", pd->parsed_size, hdr_count, hdr_count != 1 ? "s" : "", pd->payload_size);
#[         } else {
#[             debug(" " T4LIT(%%%%%%%%,success) " Packet is " T4LIT(accepted,success) ", " T4LIT(%d) "B in " T4LIT(%d) " header%s, " T4LIT(empty payload) "\n", pd->parsed_size, hdr_count, hdr_count != 1 ? "s" : "");
#}         }
#[
#{         if (hdr_count == 0) {
#[             debug("   " T4LIT(!! No headers were found,warning) " during parsing, the packet solely consists of payload\n");
#}         }
#}     #endif
#} }
#[

for parser in hlir.parsers:
    for s in parser.states:
        if not s.is_reachable:
            continue

        #{ void parser_state_${parser.name}_${s.name}(STDPARAMS) {
        #{     #ifdef T4P4S_STATS
        #[         t4p4s_stats_global.T4STAT(parser,state,${s.name}) = true;
        #[         t4p4s_stats_per_packet.T4STAT(parser,state,${s.name}) = true;
        #}     #endif
        #[

        if s.name == 'accept':
            #[     pd->payload_size = packet_size(pd) - (pd->extract_ptr - (void*)pd->data);
            #[     print_parsing_success(STDPARAMS_IN);
        elif s.name == 'reject':
            #[     debug(" " T4LIT(XXXX,status) " Parser state " T4LIT(${s.name},parserstate) " %s\n", transition_cond);
            #[     debug("   " T4LIT(XX,status) " Packet is " T4LIT(dropped,status) "\n");
            #[     drop_packet(STDPARAMS_IN);
        else:
            #[     debug(" %%%%%%%% Parser state " T4LIT(${s.name},parserstate) "%s\n", transition_cond);
            #[     set_transition_txt("");

            for idx, component in enumerate(s.components):
                #[     bool success$idx = ${state_component_name(parser.name, s, idx, component)}(STDPARAMS_IN);
                #{     if (unlikely(!success$idx)) {
                #[         debug("    " T4LIT(!,error) " Parsing " T4LIT(failed,error) ", " T4LIT(dropping,status) " packet\n");
                #[         return;
                #}     }

            if s.selectExpression.node_type == 'PathExpression':
                #[     parser_state_${parser.name}_${s.selectExpression.path.name}(STDPARAMS_IN);
            else:
                #[     parser_state_${parser.name}_${s.name}_next_state(STDPARAMS_IN);

        #} }
        #[

all_stk_hdrs = hlir.header_instances.filter(lambda hdr: 'stack' in hdr)
for stk in unique_everseen(all_stk_hdrs.map('stack')):
    #{ int parser_extract_${stk.name}(int vwlen, STDPARAMS) {
    #{     switch (pd->stacks[STK(${stk.name})].current) {

    for idx, stkhdr in enumerate(all_stk_hdrs.filter('stack', stk)):
        #[         case ${idx-1}: return parser_extract_${stkhdr.name}(vwlen, STDPARAMS_IN);
    #[         default:     return PARSED_OVER_STACK_SIZE; // cannot be reached
    #}     }
    #} }
    #[
