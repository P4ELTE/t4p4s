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
from utils.codegen import format_expr, format_statement, statement_buffer_value, format_declaration


#[ #include "dpdk_lib.h"

#[ extern int get_var_width_bitwidth();

def header_bit_width(h):
    return sum([f.size if not f.is_vw else 0 for f in h.type.fields])

def gen_extract_header_tmp(h):
    #[ memcpy(pstate->${h.ref.name}, buf, ${h.type.byte_width});
    #[ buf += ${h.type.byte_width};
    #[ pd->parsed_length += ${h.type.byte_width};

def gen_extract_header_tmp_2(h, w):
    x = header_bit_width(h)
    w = format_expr(w)
    #[ int hdrlen = ((${w}+${x})/8);
    #[ pd->parsed_length += hdrlen;
    #[ memcpy(pstate->${h.ref.name}, buf, hdrlen);
    #[ pstate->${h.ref.name}_var += ${w}+${x};
    #[ buf += hdrlen;


def gen_extract_header(h):
    #[ if((int)((uint8_t*)buf-(uint8_t*)(pd->data))+${h.type.type_ref.byte_width} > pd->wrapper->pkt_len); // packet_too_short // TODO optimize this
    #[ pd->headers[${h.id}].pointer = buf;
    #[ pd->headers[${h.id}].length = ${h.type.type_ref.byte_width};
    #[ pd->parsed_length += ${h.type.type_ref.byte_width};
    #[ buf += pd->headers[${h.id}].length;
    for f in h.type.type_ref.fields:
        # TODO get rid of "f.get_attr('preparsed') is not None"
        # TODO (f must always have a preparsed attribute)
        if f.get_attr('preparsed') is not None and f.preparsed and f.size <= 32:
            #[ EXTRACT_INT32_AUTO_PACKET(pd, ${h.id}, ${f.id}, value32)
            #[ pd->fields.${f.id} = value32;
            #[ pd->fields.attr_${f.id} = 0;

def gen_extract_header_2(h, w):
    if not h.type.is_vw:
        addError("generating extract header call", "fixed-width header extracted with two-param extract")
    else:
        x = header_bit_width(h)
        w = format_expr(w)
        #[ int hdrlen = ((${w}+${x})/8);

        #[ if((int)((uint8_t*)buf-(uint8_t*)(pd->data))+hdrlen > pd->wrapper->pkt_len); // packet_too_short // TODO optimize this
        #[ if(hdrlen > ${h.type.byte_width}); // header_too_long
        #[ pd->headers[${h.id}].pointer = buf;
        #[ pd->headers[${h.id}].length = hdrlen;
        #[ pd->parsed_length += hdrlen;
        #[ pd->headers[${h.id}].var_width_field_bitwidth = hdrlen * 8 - ${header_bit_width(h)};


################################################################################

parser = hlir16.declarations['P4Parser'][0]

for l in parser.parserLocals:
    #[ ${format_declaration(l)}

for s in parser.states:
    #[ static void parser_state_${s.name}(packet_descriptor_t* pd, uint8_t* buf, lookup_table_t** tables, parser_state_t* pstate);

for s in parser.states:
    if s.node_type != 'ParserState': continue

    #[ static void parser_state_${s.name}(packet_descriptor_t* pd, uint8_t* buf, lookup_table_t** tables, parser_state_t* pstate) {
    #[     uint32_t value32; (void)value32;
    #[     uint32_t res32; (void)res32;
    #[     debug(" :::: Parser state $$[parserstate]{s.name}\n");

    for c in s.components:
        if hasattr(c, 'call'):
            if c.call == 'extract_header':
                hdrtype = c.header.type.type_ref if hasattr(c.header.type, 'type_ref') else c.header.type
                bitwidth = hdrtype.bit_width if not c.is_vw else header_bit_width(c.header)

                if not c.is_tmp:
                    if not c.is_vw:
                        #[ ${gen_extract_header(c.header)}
                    else:
                        #[ ${gen_extract_header_2(c.header, c.width)}
                    #[ dbg_bytes(pd->headers[${c.header.id}].pointer, pd->headers[${c.header.id}].length,
                    #[           "   :: Extracted header $$[header]{c.header.name} ($${(bitwidth+7)/8}{ bytes}): ");
                else:
                    if not c.is_vw:
                        #[ ${gen_extract_header_tmp(c.header)}
                        #[ dbg_bytes(pstate->${c.header.ref.name}, ${c.header.type.byte_width},
                        #[           "   :: Extracted header $$[header]{c.header.path.name} of type $${c.header.type.name} ($${bitwidth} bits, $${c.header.type.byte_width} bytes): ");

                    else:
                        #[ ${gen_extract_header_tmp_2(c.header, c.width)}
                        hdr_width = header_bit_width(c.header)
                        var_width = format_expr(c.width)
                        #[ dbg_bytes(pstate->${c.header.ref.name}, (($hdr_width + $var_width)+7)/8,
                        #[           "   :: Extracted header $$[header]{c.header.path.name} of type $${c.header.type.name}: ($${hdr_width}+$${}{%d} bits, $${}{%d} bytes): ",
                        #[           $var_width, (($hdr_width + $var_width)+7)/8);
        else:
            #[ ${format_statement(c)}

    if not hasattr(s, 'selectExpression'):
        if s.name == 'accept':
            #[ debug("   :: Packet is $$[success]{}{accepted}\n");
        if s.name == 'reject':
            #[ debug("   :: Packet is $$[success]{}{dropped}\n");
            #[ pd->dropped = 1;
    else:
        b = s.selectExpression
        if b.node_type == 'PathExpression':
            x = "parser_state_" + format_expr(b) + "(pd, buf, tables, pstate);"
        if b.node_type == 'SelectExpression':
            x = format_expr(b)

        prebuf, postbuf = statement_buffer_value()

        #[ $prebuf
        #[ $x
        #[ $postbuf
    #[ }

#[ void parse_packet(packet_descriptor_t* pd, lookup_table_t** tables, parser_state_t* pstate) {
#[     parser_state_start(pd, pd->data, tables, pstate);
#[ }


#{ const char* header_instance_names[HEADER_INSTANCE_COUNT] = {
for hdr in hlir16.header_instances:
    #[ "${hdr.name}", // header_instance_${hdr.name}
#} };

#{ const char* field_names[FIELD_COUNT] = {
for hdr in hlir16.header_types:
    for fld in hdr.fields:
        #[ "${fld.name}", // field_instance_${hdr.name}_${fld.name}
#} };


#[ // Returns the sum of all collected variable widths,
#[ // and resets all varwidth counters.
#{ int get_var_width_bitwidth(parser_state_t* pstate) {
#[     int retval = 0
for loc in parser.parserLocals:
    #[ + pstate->${loc.name}_var
#[     ;

for loc in parser.parserLocals:
    #[ pstate->${loc.name}_var = 0;

#[ return retval;

#} }
