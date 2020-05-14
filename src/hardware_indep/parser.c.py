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


def gen_extract_header(hdrinst, hdrtype, arg0_expr):
    if hdrinst is None:
        addError("extracting header", "no instance found for header type " + hdrtype.name)
        return

    if hasattr(arg0_expr, "path"):
        hdrname = arg0_expr.path.name
    else:
        hdrname = hdrinst.name

    #[ if(unlikely((int)((uint8_t*)buf-(uint8_t*)(pd->data))+${hdrtype.byte_width} > pd->wrapper->pkt_len))
    #[     ; // packet_too_short // TODO optimize this
    #[ pd->headers[header_instance_${hdrname}].pointer = buf;
    #[ pd->headers[header_instance_${hdrname}].was_enabled_at_initial_parse = true;
    #[ pd->headers[header_instance_${hdrname}].length = ${hdrtype.byte_width};
    #[ pd->parsed_length += ${hdrtype.byte_width};
    for f in hdrtype.fields:
        # TODO get rid of "f.get_attr('preparsed') is not None"
        # TODO (f must always have a preparsed attribute)
        if f.get_attr('preparsed') is not None and f.preparsed and f.size <= 32:
            #[ EXTRACT_INT32_AUTO_PACKET(pd, ${hdrinst.id}, ${f.id}, value32)
            #[ pd->fields.${f.id} = value32;
            #[ pd->fields.attr_${f.id} = 0;
    #[ buf += ${hdrtype.byte_width};

def gen_extract_header_var_width(hdrinst, hdrtype, width_expr):
    if not hdrtype.is_vw:
        addError("generating extract header call", "fixed-width header extracted with two-param extract")
        return

    x = header_bit_width(hdrtype)

    #[ uint32_t hdrlen = ((${format_expr(width_expr)}+${x})/8);
    #[ if (unlikely((int)((uint8_t*)buf-(uint8_t*)(pd->data))+hdrlen > pd->wrapper->pkt_len))
    #[     ; // packet_too_short // TODO optimize this
    #[ if (hdrlen > ${hdrtype.byte_width})
    #[     debug("    " T4LIT(!,warning) " variable width header " T4LIT(${hdrinst.name},header) " is " T4LIT(too long,warning) " (" T4LIT(%d,warning) " bytes)\n", hdrlen);
    #[ pd->headers[${hdrinst.id}].pointer = buf;
    #[ pd->headers[${hdrinst.id}].was_enabled_at_initial_parse = true;
    #[ pd->headers[${hdrinst.id}].length = hdrlen;
    #[ pd->parsed_length += hdrlen;
    #[ pd->headers[${hdrinst.id}].var_width_field_bitwidth = hdrlen * 8 - ${header_bit_width(hdrtype)};
    #[ buf += hdrlen;


################################################################################

# TODO more than one parser can be present
parser = hlir16.objects['P4Parser'][0]


#{ void init_parser_state(parser_state_t* pstate) {
for l in parser.parserLocals:
    if l.node_type == 'Declaration_Instance':
        #[ ${l.type.type_ref.name}_t_init(pstate->${l.name});
#} }

for s in parser.states:
    #[ static void parser_state_${s.name}(packet_descriptor_t* pd, uint8_t* in_buf, lookup_table_t** tables, parser_state_t* pstate);

for s in parser.states:
    #[ static void parser_state_${s.name}(packet_descriptor_t* pd, uint8_t* in_buf, lookup_table_t** tables, parser_state_t* pstate) {
    #[     uint32_t value32; (void)value32;
    #[     uint32_t res32; (void)res32;
    #[     parser_state_t* local_vars = pstate;
    #[     uint8_t* buf = in_buf;
    #[     debug(" :::: Parser state $$[parserstate]{s.name}\n");

    for c in s.components:
        if not hasattr(c, 'call'):
            #[ ${format_statement(c, parser)}
            continue

        if c.call != 'extract_header':
            continue

        #[ // Extracting header ${s.name}

        hdrtype = c.header.type_ref if hasattr(c.header, 'type_ref') else c.header

        arg0_expr = c.methodCall.arguments['Argument'][0]

        # TODO find a more universal way to get to the header instance
        if hasattr(arg0_expr, 'path'):
            hdrinst_name = arg0_expr.path.name

            dvar = parser.parserLocals.get(hdrinst_name, 'Declaration_Variable')
            if dvar:
                insts = [hi for hi in hlir16.header_instances['StructField'] if hi.type.type_ref.name == hdrtype.name]
                if len(insts) == 1:
                    hdrinst = insts[0]
                elif len(insts) == 0:
                    # note: it is defined as a local variable
                    hdrinst = dvar.type.type_ref
                else:
                    addError("Finding header instance", "There is no single header instance that corresponds to {}".format(hdrtype.name))

                hdrtype = dvar.type.type_ref
            else:
                hdrinst = hlir16.header_instances.get(hdrinst_name, 'Declaration_Variable', lambda hi: hi.type.type_ref.name == hdrtype.name)

            pstate_var_name = dvar.name
        elif hasattr(c.methodCall.method.expr, 'header_ref'):
            hdrinst = c.methodCall.method.expr.header_ref
            pstate_var_name = hdrinst.name
        else:
            arg0_expr = c.methodCall.arguments[0].expression
            if hasattr(arg0_expr, "member"):
                hdrinst_name = arg0_expr.member
                hdrinst = hlir16.header_instances.get(hdrinst_name, 'StructField', lambda hi: hi.type.type_ref.name == hdrtype.name)
            else:
                hdrinst_name = arg0_expr.path.name
                hdrinst = hlir16.header_instances.get(hdrinst_name)
            pstate_var_name = hdrinst.name

        # TODO there should be no "secondary" hdrtype node
        if not hasattr(hdrtype, 'bit_width'):
            hdrtype = hlir16.header_types.get(hdrtype.name, 'Type_Header')

        bitwidth = hdrtype.bit_width if not c.is_vw else header_bit_width(hdrtype)

        if not c.is_tmp:
            if not c.is_vw:
                #[ ${gen_extract_header(hdrinst, hdrtype, arg0_expr)}
            else:
                #[ ${gen_extract_header_var_width(hdrinst, hdrtype, c.width)}

                # the variable counts in bytes, length in bits, hence the multiplication by 8
                #[ pstate->${pstate_var_name}_var = pd->headers[${hdrinst.id}].length * 8;
            #[ dbg_bytes(pd->headers[${hdrinst.id}].pointer, pd->headers[${hdrinst.id}].length,
            #[           "   :: Extracted ${"variable width " if c.is_vw else ""}header instance " T4LIT(#%d) " $$[header]{hdrinst.name}/$${}{%d}B: ", ${hdrinst.id} + 1, pd->headers[${hdrinst.id}].length);
        else:
            if not c.is_vw:
                #[ ${gen_extract_header_tmp(hdrinst)}
                #[ dbg_bytes(pstate->${hdrinst.ref.name}, ${hdrtype.byte_width},
                #[           "   :: Extracted header instance " T4LIT(#%d) " $$[header]{hdrinst.path.name} of type $${hdrinst.type.name}/$${hdrtype.byte_width}B: ", ${hdrinst.id} + 1, pd->headers[${hdrinst.id}].length);

            else:
                #[ ${gen_extract_header_tmp_2(hdrinst, hdrtype, c.width)}
                hdr_width = header_bit_width(hdrtype)
                var_width = format_expr(c.width)
                #[ dbg_bytes(pstate->${hdrinst.ref.name}, (($hdr_width + $var_width)+7)/8,
                #[           "   :: Extracted header instance " T4LIT(#%d) " $$[header]{hdrinst.path.name} of type $${hdrtype.name}/$${}{%d}B: ",
                #[           ${hdrinst.id} + 1, (($hdr_width + $var_width)+7)/8);

    if not hasattr(s, 'selectExpression'):
        if s.name == 'accept':
            #[ debug("   :: Packet is $$[success]{}{accepted}\n");
            #[ pd->payload_length = packet_length(pd) - pd->parsed_length;

            #{ if (pd->payload_length > 0) {
            #[     dbg_bytes(pd->data + pd->parsed_length, pd->payload_length, "    : " T4LIT(Payload,header) " is $${}{%d} bytes: ", pd->payload_length);
            #[ } else {
            #[     debug("    : " T4LIT(Payload,header) " is empty\n");
            #} }
        if s.name == 'reject':
            #[ debug("   :: Packet is $$[status]{}{dropped}\n");
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


#[ // Returns the sum of all collected variable widths.
#{ int get_var_width_bitwidth(parser_state_t* pstate) {
#[     int retval = 0
for loc in parser.parserLocals:
    #[ + pstate->${loc.name}_var
#[     ;

#[ return retval;

#} }
