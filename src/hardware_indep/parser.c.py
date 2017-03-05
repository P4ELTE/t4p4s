# Copyright 2016 Eotvos Lorand University, Budapest, Hungary
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import p4_hlir.hlir.p4 as p4
from utils.hlir import *
from utils.misc import addError, addWarning 

def format_state(state):
    generated_code = ""
    if isinstance(state, p4.p4_parse_state):
        #[ return parse_state_${state.name}(pd, buf, tables);
    elif isinstance(state, p4.p4_parser_exception):
        print "Parser exceptions are not supported yet."
    else: #Control function (parsing is finished)
        #[ {
        #[   if(verify_packet(pd)) p4_pe_checksum(pd);
        #[   ${format_p4_node(state)}
        #[ }
    return generated_code

def get_key_byte_width(branch_on):
    """
    :param branch_on: list of union(p4_field, tuple)
    :rtype:           int
    """
    key_width = 0
    for switch_ref in branch_on:
        if type(switch_ref) is p4.p4_field:
            key_width += (switch_ref.width+7)/8
        elif type(switch_ref) is tuple:
            key_width += max(4, (switch_ref[1] + 7) / 8)
    return key_width

pe_dict = { "p4_pe_index_out_of_bounds" : None,
            "p4_pe_out_of_packet" : None,
            "p4_pe_header_too_long" : None,
            "p4_pe_header_too_short" : None,
            "p4_pe_unhandled_select" : None,
            "p4_pe_checksum" : None,
            "p4_pe_default" : None }

pe_default = p4.p4_parser_exception(None, None)
pe_default.name = "p4_pe_default"
pe_default.return_or_drop = p4.P4_PARSER_DROP

for pe_name, pe in pe_dict.items():
    pe_dict[pe_name] = pe_default
for pe_name, pe in hlir.p4_parser_exceptions.items():
    pe_dict[pe_name] = pe

#[ #include "dpdk_lib.h"
#[ #include "actions.h" // apply_table_* and action_code_*
#[
#[ extern int verify_packet(packet_descriptor_t* pd);
#[
#[ void print_mac(uint8_t* v) { printf("%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX\n", v[0], v[1], v[2], v[3], v[4], v[5]); }
#[ void print_ip(uint8_t* v) { printf("%d.%d.%d.%d\n",v[0],v[1],v[2],v[3]); }
#[ 
#[ static void
#[ extract_header(uint8_t* buf, packet_descriptor_t* pd, header_instance_t h) {
#[     pd->headers[h] =
#[       (header_descriptor_t) {
#[         .type = h,
#[         .pointer = buf,
#[         .length = header_instance_byte_width[h],
#[         .var_width_field_bitwidth = 0,
#[         .var_width_field_excess_byte_count = 0
#[       };
#[ }
#[ 

for pe_name, pe in pe_dict.items():
    #[ static inline void ${pe_name}(packet_descriptor_t *pd) {
    if pe.return_or_drop == p4.P4_PARSER_DROP:
        #[ pd->dropped = 1;
    else:
        format_p4_node(pe.return_or_drop)
    #[ }

for state_name, parse_state in hlir.p4_parse_states.items():
    #[ static void parse_state_${state_name}(packet_descriptor_t* pd, uint8_t* buf, lookup_table_t** tables);
#[

for state_name, parse_state in hlir.p4_parse_states.items():
    branch_on = parse_state.branch_on
    if branch_on:
        #[ static inline void build_key_${state_name}(packet_descriptor_t *pd, uint8_t *buf, uint8_t *key) {
        for switch_ref in branch_on:
            if type(switch_ref) is p4.p4_field:
                field_instance = switch_ref
                byte_width = (field_instance.width + 7) / 8
                if byte_width <= 4:
                    #[ EXTRACT_INT32_BITS(pd, ${fld_id(field_instance)}, *(uint32_t*)key)
                    #[ key += sizeof(uint32_t);
                else:
                    #[ EXTRACT_BYTEBUF(pd, ${fld_id(field_instance)}, key)
                    #[ key += ${byte_width};
            elif type(switch_ref) is tuple:
                #[     uint8_t* ptr;
                offset, width = switch_ref
                # TODO
                addError("generating parse state %s"%state_name, "current() calls are not supported yet")
        #[ }

for state_name, parse_state in hlir.p4_parse_states.items():
    #[ static void parse_state_${state_name}(packet_descriptor_t* pd, uint8_t* buf, lookup_table_t** tables)
    #[ {
    #[     uint32_t value32;
    #[     (void)value32;
    
    for call in parse_state.call_sequence:
        if call[0] == p4.parse_call.extract:
            hi = call[1] 
            #[     extract_header(buf, pd, ${hdr_prefix(hi.name)});
            if isinstance(hi.header_type.length, p4.p4_expression):
                #[     pd->headers[${hdr_prefix(hi.name)}].length = ${format_expr(resolve_field_ref(hlir, hi, hi.header_type.length))};
                #[     pd->headers[${hdr_prefix(hi.name)}].var_width_field_bitwidth = pd->headers[${hdr_prefix(hi.name)}].length * 8 - ${sum([f[1] if f[1] != p4.P4_AUTO_WIDTH else 0 for f in hi.header_type.layout.items()])};
                #[     pd->headers[${hdr_prefix(hi.name)}].var_width_field_excess_byte_count = (pd->headers[${hdr_prefix(hi.name)}].var_width_field_bitwidth) / 8;
                #[     buf += pd->headers[${hdr_prefix(hi.name)}].length;
            else:
                #[     buf += header_instance_byte_width[${hdr_prefix(hi.name)}];
            for f in hi.fields:
                if parsed_field(hlir, f):
                    if f.width <= 32:
                        #[ EXTRACT_INT32_AUTO(pd, ${fld_id(f)}, value32)
                        #[ pd->fields.${fld_id(f)} = value32;
                        #[ pd->fields.attr_${fld_id(f)} = 0;
        elif call[0] == p4.parse_call.set:
            dest_field, src = call[1], call[2]
            if type(src) is int or type(src) is long:
                hex(src)
                # TODO
            elif type(src) is p4.p4_field:
                src
                # TODO
            elif type(src) is tuple:
                offset, width = src
                # TODO
            addError("generating parse state %s"%state_name, "set_metadata during parsing is not supported yet")

    branch_on = parse_state.branch_on
    if not branch_on:
        branch_case, next_state = parse_state.branch_to.items()[0]
        #[ ${format_state(next_state)}
    else:
        key_byte_width = get_key_byte_width(branch_on)
        #[ uint8_t key[${key_byte_width}];
        #[ build_key_${state_name}(pd, buf, key);
        has_default_case = False
        for case_num, case in enumerate(parse_state.branch_to.items()):
            branch_case, next_state = case
            mask_name  = "mask_value_%d" % case_num
            value_name  = "case_value_%d" % case_num
            if branch_case == p4.P4_DEFAULT:
                has_default_case = True
                #[ ${format_state(next_state)}
                continue
            if type(branch_case) is int:
                value = branch_case
                value_len, l = int_to_byte_array(value)
                #[     uint8_t ${value_name}[${value_len}] = {
                for c in l:
                    #[         ${c},
                #[     };
                #[     if ( memcmp(key, ${value_name}, ${value_len}) == 0)
                #[         ${format_state(next_state)}
            elif type(branch_case) is tuple:
                value = branch_case[0]
                mask = branch_case[1]
                # TODO
                addError("generating parse state %s"%state_name, "value masking is not supported yet")
            elif type(branch_case) is p4.p4_parse_value_set:
                value_set = branch_case
                # TODO
                addError("generating parse state %s"%state_name, "value sets are not supported yet")
                continue
        if not has_default_case:
            #[     return NULL;
    #[ }
    #[ 

#[ void parse_packet(packet_descriptor_t* pd, lookup_table_t** tables) {
#[     parse_state_start(pd, pd->data, tables);
#[ }
