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

from utils.misc import addError, addWarning 
from utils.codegen import format_expr_16, format_statement_16, statement_buffer_value, format_declaration_16


#[ #include "dpdk_lib.h"
#[
#[ void print_mac(uint8_t* v) { printf("%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX\n", v[0], v[1], v[2], v[3], v[4], v[5]); }
#[ void print_ip(uint8_t* v) { printf("%d.%d.%d.%d\n",v[0],v[1],v[2],v[3]); }
#[


def extract_header_tmp(h):
    generated_code = ""
    #[ memcpy(${h.ref.name}, buf, ${h.type.byte_width});
    #[ buf += ${h.type.byte_width};
    return generated_code

def extract_header_tmp_2(h, w):
    generated_code = ""
    x = sum([f.size if not f.is_vw else 0 for f in h.type.fields])
    w = format_expr_16(w)
    #[ int hdrlen = ((${w}+${x})/8);
    #[ memcpy(${h.ref.name}, buf, hdrlen);
    #[ ${h.ref.name}_var = ${w};
    #[ buf += hdrlen;
    return generated_code

def extract_header(h):
    generated_code = ""
    #[ if((int)((uint8_t*)buf-(uint8_t*)(pd->data))+${h.type.type_ref.byte_width} > pd->wrapper->pkt_len); // packet_too_short // TODO optimize this
    #[ pd->headers[${h.id}].pointer = buf;
    #[ pd->headers[${h.id}].length = ${h.type.type_ref.byte_width};
    #[ buf += pd->headers[${h.id}].length;
    #[ // pd->headers[${h.id}].valid = 1;
    for f in h.type.type_ref.fields:
        if f.preparsed and f.size <= 32:
            #[ EXTRACT_INT32_AUTO_PACKET(pd, ${h.id}, ${f.id}, value32)
            #[ pd->fields.${f.id} = value32;
            #[ pd->fields.attr_${f.id} = 0;
    return generated_code

def extract_header_2(h, w):
    generated_code = ""
    if not h.type.is_vw:
        addError("generating extract header call", "fixed-width header extracted with two-param extract")
    else:
        x = sum([f.size if not f.is_vw else 0 for f in h.type.fields])
        w = format_expr_16(w)
        #[ int hdrlen = ((${w}+${x})/8);

        #[ if((int)((uint8_t*)buf-(uint8_t*)(pd->data))+hdrlen > pd->wrapper->pkt_len); // packet_too_short // TODO optimize this
        #[ if(hdrlen > ${h.type.byte_width}); // header_too_long
        #[ pd->headers[${h.id}].pointer = buf;
        #[ pd->headers[${h.id}].length = hdrlen;
        #[ pd->headers[${h.id}].var_width_field_bitwidth = hdrlen * 8 - ${sum([f.size if not f.is_vw else 0 for f in h.type.fields])};
        #[ // pd->headers[${h.id}].valid = 1;
    return generated_code


################################################################################

#[ /*
#[ static void parser_state_parse_ipv4(packet_descriptor_t* pd, uint8_t* buf, lookup_table_t** tables) {
#[     uint32_t value32;
#[     extract_header_ipv4(buf, pd);
#[     buf += pd->headers[header_instance_ipv4].length;
#[     EXTRACT_INT32_AUTO_PACKET(pd, header_instance_ipv4, field_ipv4_t_ttl, value32)
#[     pd->fields.field_instance_ipv4_ttl = value32;
#[     pd->fields.attr_field_instance_ipv4_ttl = 0;
#[ }
#[ */

parser = hlir16.declarations['P4Parser'][0]

for l in parser.parserLocals:
    #[ ${format_declaration_16(l)}

for s in parser.states:
    #[ static void parser_state_${s.name}(packet_descriptor_t* pd, uint8_t* buf, lookup_table_t** tables);

for s in parser.states:
    if s.node_type != 'ParserState': continue
    #if s.name == 'parse_ipv4': continue # skipping parse state for ipv4...

    #[ static void parser_state_${s.name}(packet_descriptor_t* pd, uint8_t* buf, lookup_table_t** tables) {
    #[     uint32_t value32; (void)value32;
    #[     uint32_t res32; (void)res32;
    #[     debug("entering parser state ${s.name}...\n");
    for c in s.components:
        if hasattr(c, 'call'):
            if c.call == 'extract_header':
                if not c.is_tmp:
                    if not c.is_vw:
                        #[ ${extract_header(c.header)}
                    else:
                        #[ ${extract_header_2(c.header, c.width)}
                else:
                    if not c.is_vw:
                        #[ ${extract_header_tmp(c.header)}
                    else:
                        #[ ${extract_header_tmp_2(c.header, c.width)}
        else:
            #[ ${format_statement_16(c)}

    if not hasattr(s, 'selectExpression'):
        if s.name == 'accept':
            #[ // accepted
        if s.name == 'reject':
            #[ pd->dropped = 1;
            #[ // rejected
    else:
        b = s.selectExpression
        if b.node_type == 'PathExpression':
            x = "parser_state_" + format_expr_16(b) + "(pd, buf, tables);"
        if b.node_type == 'SelectExpression':
            x = format_expr_16(b)
        #[ ${statement_buffer_value()}
        #[ ${x}
    #[ }

#[ void parse_packet(packet_descriptor_t* pd, lookup_table_t** tables) {
#[     parser_state_start(pd, pd->data, tables);
#[ }

