# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from utils.misc import addError, addWarning 
from utils.codegen import format_expr, format_statement, format_declaration
from compiler_common import statement_buffer_value


#[ #include "dpdk_lib.h"
#[ #include "util_packet.h"

#[ extern int get_var_width_bitwidth();

def header_bit_width(hdrtype):
    return sum((f.size if not f.is_vw else 0 for f in hdrtype.fields))

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
        addError("extracting header", f"no instance found for header type {hdrtype.name}")
        return

    if 'path' in arg0_expr:
        hdrname = arg0_expr.path.name
    else:
        hdrname = hdrinst.name

    #[ if(unlikely((int)((uint8_t*)buf-(uint8_t*)(pd->data))+${hdrtype.byte_width} > pd->wrapper->pkt_len))
    #[     ; // packet_too_short // TODO optimize this
    #[ pd->headers[HDR(${hdrname})].pointer = buf;
    #[ pd->headers[HDR(${hdrname})].was_enabled_at_initial_parse = true;
    #[ pd->headers[HDR(${hdrname})].length = ${hdrtype.byte_width};
    #[ pd->parsed_length += ${hdrtype.byte_width};
    for f in hdrtype.fields:
        # TODO get rid of "f.get_attr('preparsed') is not None"
        # TODO (f must always have a preparsed attribute)
        if f.preparsed and f.size <= 32:
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
parser = hlir.parsers[0]


#{ void init_parser_state(parser_state_t* pstate) {
for l in parser.parserLocals:
    if l.node_type == 'Declaration_Instance':
        #[ ${l.urtype.name}_t_init(pstate->${l.name});
#} }

for s in parser.states:
    #[ static void parser_state_${s.name}(packet_descriptor_t* pd, uint8_t* in_buf, lookup_table_t** tables, parser_state_t* pstate);

# TODO find a more universal way to get to the header instance
def get_hdrinst(arg0_expr, c):
    if 'path' in arg0_expr:
        hdrinst_name = arg0_expr.path.name

        hdrtype = c.header.urtype
        dvar = parser.parserLocals.get(hdrinst_name, 'Declaration_Variable')
        if dvar:
            insts = [hi for hi in hlir.header_instances['StructField'] if hi.urtype.name == hdrtype.name]
            if len(insts) == 1:
                hdrinst = insts[0]
            elif len(insts) == 0:
                # note: it is defined as a local variable
                hdrinst = dvar.urtype
            else:
                addError("Finding header instance", f"There is no single header instance that corresponds to {hdrtype.name}")
        else:
            hdrinst = hlir.header_instances.get(hdrinst_name, 'Declaration_Variable', lambda hi: hi.urtype.name == hdrtype.name)

        return hdrinst, dvar.name
    elif 'header_ref' in c.methodCall.method.expr:
        hdrinst = c.methodCall.method.expr.header_ref
        return hdrinst, hdrinst.name
    else:
        arg0_expr = c.methodCall.arguments[0].expression
        if "member" in arg0_expr:
            hdrtype = c.header.urtype
            hdrinst_name = arg0_expr.member
            hdrinst = hlir.header_instances.get(hdrinst_name, 'StructField', lambda hi: hi.urtype.name == hdrtype.name)
        else:
            hdrinst_name = arg0_expr.ref.name
            hdrinst = hlir.header_instances.get(hdrinst_name)
        return hdrinst, hdrinst.name

for s in parser.states:
    #{ static void parser_state_${s.name}(packet_descriptor_t* pd, uint8_t* in_buf, lookup_table_t** tables, parser_state_t* pstate) {
    #[     uint32_t value32; (void)value32;
    #[     uint32_t res32; (void)res32;
    #[     parser_state_t* local_vars = pstate;
    #[     uint8_t* buf = in_buf;
    #[     debug(" :::: Parser state $$[parserstate]{s.name}\n");

    for c in s.components:
        if 'call' not in c:
            #[ ${format_statement(c, parser)}
            continue

        if c.call != 'extract_header':
            continue

        #[ // Extracting header ${s.name}

        arg0_expr = c.methodCall.arguments['Argument'][0]

        hdrinst, pstate_var_name = get_hdrinst(arg0_expr, c)
        hdrtype = hdrinst.urtype

        bitwidth = hdrtype.size if not c.is_vw else header_bit_width(hdrtype)

        if not c.is_tmp:
            if not c.is_vw:
                #[ ${gen_extract_header(hdrinst, hdrtype, arg0_expr)}
            else:
                #[ ${gen_extract_header_var_width(hdrinst, hdrtype, c.width)}

                # the variable counts in bytes, length in bits, hence the multiplication by 8
                #[ pstate->${pstate_var_name}_var = pd->headers[${hdrinst.id}].length * 8;
            #[ dbg_bytes(pd->headers[${hdrinst.id}].pointer, pd->headers[${hdrinst.id}].length,
            #[           "   :: Extracted ${"variable width " if c.is_vw else ""}header instance " T4LIT(#%d) " $$[header]{hdrinst.name}/$${}{%d}B: ", hdr_infos[${hdrinst.id}].idx, pd->headers[${hdrinst.id}].length);
        else:
            hdrname = hdrinst.urtype.path.name
            if not c.is_vw:
                #[ ${gen_extract_header_tmp(hdrinst)}
                #[ dbg_bytes(pstate->${hdrinst.ref.name}, ${hdrtype.byte_width},
                #[           "   :: Extracted header instance " T4LIT(#%d) " $$[header]{hdrname} of type $${hdrinst.type.name}/$${hdrtype.byte_width}B: ", hdr_infos[${hdrinst.id}].idx, pd->headers[${hdrinst.id}].length);

            else:
                #[ ${gen_extract_header_tmp_2(hdrinst, hdrtype, c.width)}
                hdr_width = header_bit_width(hdrtype)
                var_width = format_expr(c.width)
                #[ dbg_bytes(pstate->${hdrinst.ref.name}, (($hdr_width + $var_width)+7)/8,
                #[           "   :: Extracted header instance " T4LIT(#%d) " $$[header]{hdrname} of type $${hdrtype.name}/$${}{%d}B: ",
                #[           hdr_infos[${hdrinst.id}].idx, (($hdr_width + $var_width)+7)/8);

        #{ if (packet_length(pd) < pd->parsed_length) {
        #[     debug(" " T4LIT(!!!!,error) " Parse overflow: already parsed " T4LIT(%dB) ", but package is only " T4LIT(%dB) " long\n", pd->parsed_length, packet_length(pd));
        #} }

    if 'selectExpression' not in s:
        if s.name == 'accept':
            #[ debug("   " T4LIT(::,success) " Packet is $$[success]{}{accepted}, total length of headers: " T4LIT(%d) " bytes\n", pd->parsed_length);

            #[ pd->payload_length = packet_length(pd) - pd->parsed_length;

            #{ if (pd->payload_length > 0) {
            #[     dbg_bytes(pd->data + pd->parsed_length, pd->payload_length, "    : " T4LIT(Payload,header) " is $${}{%d} bytes: ", pd->payload_length);
            #[ } else {
            #[     debug("    " T4LIT(:,status) " " T4LIT(Payload,header) " is " T4LIT(empty,status) "\n");
            #} }
        if s.name == 'reject':
            #[ debug("   " T4LIT(::,status) " Packet is $$[status]{}{dropped}\n");
            #[     MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), field_instance_all_metadatas_drop, true);
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
    #} }

#[ void parse_packet(packet_descriptor_t* pd, lookup_table_t** tables, parser_state_t* pstate) {
#[     parser_state_start(pd, pd->data, tables, pstate);
#[ }


#{ const char* header_instance_names[HEADER_COUNT] = {
for hdr in hlir.header_instances:
    #[ "${hdr.name}",
#} };

#{ const char* field_names[FIELD_COUNT] = {
for hdr in hlir.hlir.header_instances:
    for fld in hdr.fields:
        #[ "${fld.name}", // in header ${hdr.name}
#} };


#[ // Returns the sum of all collected variable widths.
#{ int get_var_width_bitwidth(parser_state_t* pstate) {
#[     int retval = 0
for loc in parser.parserLocals:
    #[ + pstate->${loc.name}_var
#[     ;

#[ return retval;

#} }
