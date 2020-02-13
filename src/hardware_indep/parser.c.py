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
#[ #include "util_packet.h"

#[ extern int get_var_width_bitwidth();

def header_bit_width(hdrtype):
    return sum([f.size if not f.is_vw else 0 for f in hdrtype.fields])

def gen_extract_header_tmp(h):
    #[ memcpy(pstate->${h.ref.name}, buf, ${h.type.byte_width});
    #[ buf += ${h.type.byte_width};
    #[ pd->parsed_length += ${h.type.byte_width};

def gen_extract_header_tmp_2(hdrinst, hdrtype, w):
    x = header_bit_width(hdrtype)
    w = format_expr(w)
    #[ int hdrlen = ((${w}+${x})/8);
    #[ pd->parsed_length += hdrlen;
    #[ memcpy(pstate->${hdrinst.ref.name}, buf, hdrlen);
    #[ pstate->${hdrinst.ref.name}_var += ${w}+${x};
    #[ buf += hdrlen;


def gen_extract_header(hdrinst, hdrtype):
    if hdrinst is None:
        addError("extracting header", "no instance found for header type " + hdrtype.name)
        return

    #[ if((int)((uint8_t*)buf-(uint8_t*)(pd->data))+${hdrtype.byte_width} > pd->wrapper->pkt_len)
    #[     ; // packet_too_short // TODO optimize this
    #[ pd->headers[${hdrinst.id}].pointer = buf;
    #[ pd->headers[${hdrinst.id}].length = ${hdrtype.byte_width};
    #[ pd->parsed_length += ${hdrtype.byte_width};
    #[ buf += pd->headers[${hdrinst.id}].length;
    for f in hdrtype.fields:
        # TODO get rid of "f.get_attr('preparsed') is not None"
        # TODO (f must always have a preparsed attribute)
        if f.get_attr('preparsed') is not None and f.preparsed and f.size <= 32:
            #[ EXTRACT_INT32_AUTO_PACKET(pd, ${hdrinst.id}, ${f.id}, value32)
            #[ pd->fields.${f.id} = value32;
            #[ pd->fields.attr_${f.id} = 0;

def gen_extract_header_2(hdrinst, hdrtype, w):
    if not hdrtype.is_vw:
        addError("generating extract header call", "fixed-width header extracted with two-param extract")
        return

    x = header_bit_width(hdrtype)

    #[ uint32_t hdrlen = ((${format_expr(w)}+${x})/8);

    #[ if ((int)((uint8_t*)buf-(uint8_t*)(pd->data))+hdrlen > pd->wrapper->pkt_len)
    #[     ; // packet_too_short // TODO optimize this
    #[ if (hdrlen > ${hdrtype.byte_width})
    #[     debug("    " T4LIT(!,warning) " header " T4LIT(${hdrinst.name},header) " is too long (" T4LIT(%d,warning) " bytes)\n", hdrlen);
    #[ pd->headers[${hdrinst.id}].pointer = buf;
    #[ pd->headers[${hdrinst.id}].length = hdrlen;
    #[ pd->parsed_length += hdrlen;
    #[ pd->headers[${hdrinst.id}].var_width_field_bitwidth = hdrlen * 8 - ${header_bit_width(hdrtype)};


################################################################################

# TODO more than one parser can be present
parser = hlir16.objects['P4Parser'][0]


#{ void init_parser_state(parser_state_t* pstate) {
for l in parser.parserLocals:
    if l.node_type == 'Declaration_Instance':
        #[ ${l.type.type_ref.name}_t_init(pstate->${l.name});
#} }

for s in parser.states:
    #[ static void parser_state_${s.name}(packet_descriptor_t* pd, uint8_t* buf, lookup_table_t** tables, parser_state_t* pstate);

for s in parser.states:
    #[ static void parser_state_${s.name}(packet_descriptor_t* pd, uint8_t* buf, lookup_table_t** tables, parser_state_t* pstate) {
    #[     uint32_t value32; (void)value32;
    #[     uint32_t res32; (void)res32;
    #[     parser_state_t* local_vars = pstate;
    #[     debug(" :::: Parser state $$[parserstate]{s.name}\n");

    for c in s.components:
        if not hasattr(c, 'call'):
            #[ ${format_statement(c, parser)}
            continue

        if c.call != 'extract_header':
            continue

        #[ // CALL ${c} ${s}

        hdrtype = c.header.type_ref if hasattr(c.header, 'type_ref') else c.header

        # TODO find a more universal way to get to the header instance
        if hasattr(c.methodCall.arguments[0].expression, 'path'):
            hdrinst_name = c.methodCall.arguments['Argument'][0].expression.path.name

            dvar = parser.parserLocals.get(hdrinst_name, 'Declaration_Variable')
            if dvar:
                hdrinst = dvar.type.type_ref
                hdrtype = dvar.type.type_ref
            else:
                hdrinst = hlir16.header_instances.get(hdrinst_name, 'Declaration_Variable', lambda hi: hi.type.type_ref.name == hdrtype.name)
        elif hasattr(c.methodCall.method.expr, 'header_ref'):
            hdrinst = c.methodCall.method.expr.header_ref
        else:
            hdrinst_name = c.methodCall.arguments[0].expression.member
            hdrinst = hlir16.header_instances.get(hdrinst_name, 'StructField', lambda hi: hi.type.type_ref.name == hdrtype.name)

        # TODO there should be no "secondary" hdrtype node
        if not hasattr(hdrtype, 'bit_width'):
            hdrtype = hlir16.header_types.get(hdrtype.name, 'Type_Header')
            
        bitwidth = hdrtype.bit_width if not c.is_vw else header_bit_width(hdrtype)

        if not c.is_tmp:
            if not c.is_vw:
                #[ ${gen_extract_header(hdrinst, hdrtype)}
            else:
                #[ ${gen_extract_header_2(hdrinst, hdrtype, c.width)}
            #[ dbg_bytes(pd->headers[${hdrinst.id}].pointer, pd->headers[${hdrinst.id}].length,
            #[           "   :: Extracted header $$[header]{hdrinst.name} ($${(bitwidth+7)/8}{ bytes}): ");
        else:
            if not c.is_vw:
                #[ ${gen_extract_header_tmp(hdrinst)}
                #[ dbg_bytes(pstate->${hdrinst.ref.name}, ${hdrtype.byte_width},
                #[           "   :: Extracted header $$[header]{hdrinst.path.name} of type $${hdrinst.type.name} ($${bitwidth} bits, $${hdrtype.byte_width} bytes): ");

            else:
                #[ ${gen_extract_header_tmp_2(hdrinst, hdrtype, c.width)}
                hdr_width = header_bit_width(hdrtype)
                var_width = format_expr(c.width)
                #[ dbg_bytes(pstate->${hdrinst.ref.name}, (($hdr_width + $var_width)+7)/8,
                #[           "   :: Extracted header $$[header]{hdrinst.path.name} of type $${hdrtype.name}: ($${hdr_width}+$${}{%d} bits, $${}{%d} bytes): ",
                #[           $var_width, (($hdr_width + $var_width)+7)/8);

    if not hasattr(s, 'selectExpression'):
        if s.name == 'accept':
            #[ debug("   :: Packet is $$[success]{}{accepted}\n");
        if s.name == 'reject':
            #[ debug("   :: Packet is $$[success]{}{dropped}\n");
            #[     MODIFY_INT32_INT32_BITS_PACKET(pd, header_instance_all_metadatas, field_standard_metadata_t_drop, true);
    else:
        b = s.selectExpression
        bexpr = format_expr(b)
        prebuf, postbuf = statement_buffer_value()

        #[ $prebuf
        if b.node_type == 'PathExpression':
            #[ parser_state_$bexpr(pd, buf, tables, pstate);
        elif b.node_type == 'SelectExpression':
            #= bexpr
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
