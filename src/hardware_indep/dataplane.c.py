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
from p4_hlir.hlir.p4_sized_integer import *
from p4_hlir.hlir.p4_headers import p4_field
from utils.hlir import *
from utils.misc import addError, addWarning

#[ #include <stdlib.h>
#[ #include <string.h>
#[ #include <stdbool.h>
#[ #include "dpdk_lib.h"
#[ #include "actions.h"

#[ void mark_to_drop() {} // TODO

#[ uint16_t csum16_add(uint16_t num1, uint16_t num2) {
#[     if(num1 == 0) return num2;
#[     uint32_t tmp_num = num1 + num2;
#[     while(tmp_num > 0xffff)
#[         tmp_num = ((tmp_num & 0xffff0000) >> 16) + (tmp_num & 0xffff);
#[     return (uint16_t)tmp_num;
#[ }

#[ struct Checksum16 {
#[   uint16_t (*get) (uint8_t* data, int size, packet_descriptor_t* pd, lookup_table_t** tables);
#[ };
#[

#[ uint16_t Checksum16_get(uint8_t* data, int size, packet_descriptor_t* pd, lookup_table_t** tables) {
#[     uint32_t res = 0;
#[     res = csum16_add(res, calculate_csum16(data, size));
#[     res = (res == 0xffff) ? res : ((~res) & 0xffff);
#[     free(data);
#[     return res & ${hex((2 ** 16) - 1)};
#[ }

#[ void Checksum16_init(struct Checksum16* x) {
#[     x->get = &Checksum16_get;
#[ }

#[ extern void parse_packet(packet_descriptor_t* pd, lookup_table_t** tables);
#[ extern void increase_counter (int counterid, int index);

max_key_length = max([t.key_length_bytes for t in hlir16.tables])
#[ uint8_t reverse_buffer[${max_key_length}];

def match_type_order(t):
    if t is p4.p4_match_type.P4_MATCH_EXACT:   return 0
    if t is p4.p4_match_type.P4_MATCH_LPM:     return 1
    if t is p4.p4_match_type.P4_MATCH_TERNARY: return 2

################################################################################

STDPARAMS = "packet_descriptor_t* pd, lookup_table_t** tables"

main = hlir16.declarations['Declaration_Instance'][0] # TODO what if there are more package instances?
package_name = main.type.baseType.path.name
pipeline_elements = main.arguments

#package_type = hlir16.declarations.get(package_name, 'Type_Package')

for pe in pipeline_elements:
    c = hlir16.declarations.get(pe.type.name, 'P4Control')
    if c is not None:
        #[ void control_${pe.type.name}(${STDPARAMS});
        for t in c.controlLocals['P4Table']:
            #[ void ${t.name}_apply(${STDPARAMS});

################################################################################
# hlir16 helpers

def format_type_16(t):
    if t.node_type == 'Type_Boolean':
        return 'bool'
    if t.node_type == 'Type_Bits':
        res = 'int'
        if t.size % 8 == 0 and t.size <= 32:
            res = res + str(t.size) + '_t'
        if not t.isSigned:
            res = 'u' + res
        return res
    if t.node_type == 'Type_Name':
        return format_expr_16(t.path)

def format_declaration_16(d):
    generated_code = ""
    if d.node_type == 'Declaration_Variable':
        #[ ${format_type_16(d.type)} ${d.name};
    if d.node_type == 'Declaration_Instance':
        #[ struct ${format_type_16(d.type)} ${d.name};
        #[ ${format_type_16(d.type)}_init(&${d.name});
#        for m in d.type.ref.methods:
#            if m.name != d.type.path.name:
#                #[ ${d.name}.${m.name} = &${format_type_16(d.type)}_${m.name};
    return generated_code

def has_field_ref(node):
    return hasattr(node, 'ref') and node.ref.node_type == 'StructField' and not hasattr(node.ref, 'header_type')

def has_header_ref(node):
    return hasattr(node, 'ref') and node.ref.node_type == 'StructField' and hasattr(node.ref, 'header_type')

statement_buffer = ""

def prepend_statement(s):
    global statement_buffer
    statement_buffer += s

def format_statement_16(s):
    generated_code = ""
    global statement_buffer
    statement_buffer = ""
    if s.node_type == 'AssignmentStatement':
        if has_field_ref(s.left):
            fld_id = 'field_instance_' + s.left.expr.ref.name + '_' + s.left.ref.name
            #[ MODIFY_INT32_INT32_AUTO(pd, ${format_expr_16(s.left)}, ${format_expr_16(s.right)})
        else:
            #[ ${format_expr_16(s.left)} = ${format_expr_16(s.right)};
    if s.node_type == 'BlockStatement':
        for c in s.components:
            #[ ${format_statement_16(c)}
    if s.node_type == 'IfStatement':
        #[ if(${format_expr_16(s.condition)})
        #[ {
        if hasattr(s, 'ifTrue'):
            #[ ${format_statement_16(s.ifTrue)}
        else:
            #[ ; // true branch is not defined
        #[ } else
        #[ {
        if hasattr(s, 'ifFalse'):
            #[ ${format_statement_16(s.ifFalse)}
        else:
            #[ ; // false branch is not defined
        #[ }
    if s.node_type == 'MethodCallStatement':
        #[ ${format_expr_16(s.methodCall)};
    return statement_buffer + generated_code

def resolve_reference(e):
    if has_field_ref(e):
        h = e.expr.ref
        f = e.ref
        return (h, f)
    else:
        return e

def subsequent_fields((h1, f1), (h2, f2)):
    fs = h1.type.fields.vec
    return h1 == h2 and fs.index(f1) + 1 == fs.index(f2)

def group_references(refs):
    ret = []
    i = 0
    while i < len(refs):
        if not isinstance(refs[i], tuple):
            ret.append(refs[i])
        else:
            h, f = refs[i]
            fs = [f]
            j = 1
            while i+j < len(refs) and subsequent_fields((h, fs[-1]), refs[i+j]):
                fs.append(refs[i+j][1])
                j += 1
            i += j
            ret.append((h, fs))
    return ret

def fld_id_16(h, f):
    return 'field_instance_' + h.name + '_' + f.name

def listexpression_to_buf(expr):
    generated_code = ""
    o = '0'
    for x in group_references(map(resolve_reference, expr.components)):
        if isinstance(x, tuple):
            h, fs = x
            def width(f):
                if f.is_vw: return 'field_desc(pd, %s).bitwidth'%fld_id_16(h, f)
                else: return str(f.size)
            w = '+'.join(map(width, fs))
            #[ memcpy(buffer${expr.id} + (${o}+7)/8, field_desc(pd, ${fld_id_16(h, fs[0])}).byte_addr, (${w}+7)/8);
            o += '+'+w
    return ('int buffer%s_size = (%s+7)/8;\nuint8_t* buffer%s = calloc(buffer%s_size, sizeof(uint8_t));\n'%(expr.id, o, expr.id, expr.id)) + generated_code

def format_expr_16(e):
    if e.node_type == 'Constant':
        return str(e.value)
    if e.node_type == 'Equ':
        return format_expr_16(e.left) + '==' + format_expr_16(e.right)
    if e.node_type == 'BoolLiteral':
        return 'true' if e.value else 'false'
    if e.node_type == 'LNot':
        return '!('+format_expr_16(e.expr)+')'
    if e.node_type == 'ListExpression':
#        return ",".join(map(format_expr_16, e.components))
        prepend_statement(listexpression_to_buf(e))
        return 'buffer' + str(e.id) + ', ' + 'buffer' + str(e.id) + '_size'
    if e.node_type == 'PathExpression':
        return format_expr_16(e.path)
    if e.node_type == 'Path':
        if e.absolute:
            return "???" #TODO
        else:
            return e.name
    if e.node_type == 'Member':
        if has_field_ref(e):
            return 'field_instance_' + e.expr.ref.name + '_' + e.ref.name
        if has_header_ref(e):
            return e.ref.id
        else:
            return format_expr_16(e.expr) + '.' + e.member
    if e.node_type == 'MethodCallExpression':
        if e.method.node_type == 'Member' and e.method.member == 'emit':
            return "// packet.emit call ignored" #TODO
        elif e.method.node_type == 'Member' and e.method.member == 'isValid':
            if e.method.expr.get_attr('ref') is not None:
                return "pd->headers[%s].pointer != NULL" % e.method.expr.ref.id
            else:
                return "pd->headers[%s].pointer != NULL" % format_expr_16(e.method.expr)
        elif e.arguments.is_vec() and e.arguments.vec != [] and e.arguments[0].node_type == 'ListExpression':
            return format_expr_16(e.method) + '(' + format_expr_16(e.arguments[0]) + ', pd, tables)'
        elif e.arguments.is_vec() and e.arguments.vec != []:
            addWarning("formatting an expression", "MethodCallExpression with arguments is not properly implemented yet.")
        else:
            return format_expr_16(e.method) + '(pd, tables)'

# TODO move this to HAL
def match_type_order_16(t):
    if t == 'EXACT':   return 0
    if t == 'LPM':     return 1
    if t == 'TERNARY': return 2
    else:              return 3

################################################################################
# Table key calculation

for table in hlir16.tables:
    # TODO find out why they are missing and fix it
    #      this happens if k is a PathExpression
    if any([k.get_attr('match_type') is None for k in table.key.keyElements]):
        continue

    #[ void table_${table.name}_key(packet_descriptor_t* pd, uint8_t* key) {
    sortedfields = sorted(table.key.keyElements, key=lambda k: match_type_order_16(k.match_type))
    #TODO variable length fields
    #TODO field masks
    for f in sortedfields:
        if f.get_attr('width') is None:
            # TODO find out why this is missing and fix it
            continue
        if f.width <= 32:
            #[ EXTRACT_INT32_BITS(pd, ${f.id}, *(uint32_t*)key)
            #[ key += sizeof(uint32_t);
        elif f.width > 32 and f.width % 8 == 0:
            byte_width = (f.width+7)/8
            #[ EXTRACT_BYTEBUF(pd, ${f.id}, key)
            #[ key += ${byte_width};
        else:
            add_error("table key calculation", "Unsupported field %s ignored." % f.id)
    if table.match_type == "LPM":
        #[ key -= ${table.key_length_bytes};
        #[ int c, d;
        #[ for(c = ${table.key_length_bytes-1}, d = 0; c >= 0; c--, d++) *(reverse_buffer+d) = *(key+c);
        #[ for(c = 0; c < ${table.key_length_bytes}; c++) *(key+c) = *(reverse_buffer+c);
    #[ }

################################################################################
# Table application

for table in hlir16.tables:
    lookupfun = {'LPM':'lpm_lookup', 'EXACT':'exact_lookup', 'TERNARY':'ternary_lookup'}
    #[ void ${table.name}_apply(${STDPARAMS})
    #[ {
    #[     debug("  :::: EXECUTING TABLE ${table.name}\n");
    #[     uint8_t* key[${table.key_length_bytes}];
    #[     table_${table.name}_key(pd, (uint8_t*)key);
    #[     uint8_t* value = ${lookupfun[table.match_type]}(tables[TABLE_${table.name}], (uint8_t*)key);
    #[     struct ${table.name}_action* res = (struct ${table.name}_action*)value;
    #[     int index; (void)index;

    # COUNTERS
    # TODO

    # ACTIONS
    #[     if(res == NULL) {
    #[       debug("    :: NO RESULT, NO DEFAULT ACTION.\n");
    #[     } else {
    #[       switch (res->action_id) {
    for action in table.actions:
        action_name = action.expression.method.path.name[:-2]
        if action_name == 'NoAction':
            continue
        #[         case action_${action_name}:
        #[           debug("    :: EXECUTING ACTION ${action_name}...\n");
        if action.action_object.parameters.parameters: # action.expression.arguments != []:
            #[           action_code_${action_name}(pd, tables, res->${action_name}_params);
        else:
            #[           action_code_${action_name}(pd, tables);
        #[           break;
    #[       }
    #[     }
    #[ }
    #[
    #[ struct ${table.name}_s {
    #[     void (*apply)(packet_descriptor_t* pd, lookup_table_t** tables);
    #[ };
    #[ struct ${table.name}_s ${table.name} = {.apply = &${table.name}_apply};

################################################################################

#[ void reset_headers(packet_descriptor_t* packet_desc) {
for h in hlir16.header_instances:
    if not h.type.is_metadata:
        #[ packet_desc->headers[${h.id}].pointer = NULL;
    else:
        #[ memset(packet_desc->headers[${h.id}].pointer, 0, header_info(${h.id}).bytewidth * sizeof(uint8_t));
#[ }

#[ void init_headers(packet_descriptor_t* packet_desc) {
for h in hlir16.header_instances:
    if not h.type.is_metadata:
        #[ packet_desc->headers[${h.id}] = (header_descriptor_t)
        #[ {
        #[     .type = ${h.id},
        #[     .length = header_info(${h.id}).bytewidth,
        #[     .pointer = NULL,
        #[     .var_width_field_bitwidth = 0
        #[ };
    else:
        #[ packet_desc->headers[${h.id}] = (header_descriptor_t)
        #[ {
        #[     .type = ${h.id},
        #[     .length = header_info(${h.id}).bytewidth,
        #[     .pointer = malloc(header_info(${h.id}).bytewidth * sizeof(uint8_t)),
        #[     .var_width_field_bitwidth = 0
        #[ };
#[ }

################################################################################

#TODO are these keyless tabls supported in p4-16?

def keyless_single_action_table(table):
    return table.key_length_bytes == 0 and len(table.actions) == 2 and table.actions[1].action_object.name.startswith('NoAction')

for table in hlir16.tables:
    if keyless_single_action_table(table):
        #[ extern void ${table.name}_setdefault(struct ${table.name}_action);

#[ void init_keyless_tables() {
for table in hlir16.tables:
    if keyless_single_action_table(table):
        action = table.actions[0].action_object
        #[ struct ${table.name}_action ${table.name}_a;
        #[ ${table.name}_a.action_id = action_${action.name};
        #[ ${table.name}_setdefault(${table.name}_a);
#[ }

################################################################################

#[ void init_dataplane(${STDPARAMS}) {
#[     init_headers(pd);
#[     reset_headers(pd);
#[     init_keyless_tables();
#[     pd->dropped=0;
#[ }

#[ void update_packet(packet_descriptor_t* pd) {
#[     uint32_t value32, res32;
#[     (void)value32, (void)res32;
for f in hlir.p4_fields.values():
    if parsed_field(hlir, f):
        if f.width <= 32:
#            #[ if(pd->headers[${hdr_prefix(f.instance.name)}].pointer != NULL) {
            #[ if(pd->fields.attr_${fld_id(f)} == MODIFIED) {
            #[     value32 = pd->fields.${fld_id(f)};
            #[     MODIFY_INT32_INT32_AUTO(pd, ${fld_id(f)}, value32)
            #[ }
#[ }

################################################################################
# Pipeline

for pe in pipeline_elements:
    c = hlir16.declarations.get(pe.type.name, 'P4Control')
    if c is not None:
        #[ void control_${pe.type.name}(${STDPARAMS})
        #[ {
        #[     uint32_t value32, res32;
        #[     (void)value32, (void)res32;
        for d in c.controlLocals:
            #[ ${format_declaration_16(d)}
        #[ ${format_statement_16(c.body)}
        #[ }

#[ void process_packet(${STDPARAMS})
#[ {
for pe in pipeline_elements:
    c = hlir16.declarations.get(pe.type.name, 'P4Control')
    if c is not None:
        #[ control_${pe.type.name}(pd, tables);
        if pe.type.name == 'egress':
            #[ update_packet(pd); // we need to update the packet prior to calculating the new checksum
#[ }

################################################################################

#[ void handle_packet(${STDPARAMS})
#[ {
#[     int value32;
#[     EXTRACT_INT32_BITS(pd, field_instance_standard_metadata_ingress_port, value32)
#[     debug("### HANDLING PACKET ARRIVING AT PORT %" PRIu32 "...\n", value32);
#[     reset_headers(pd);
#[     parse_packet(pd, tables);
#[     process_packet(pd, tables);
#[ }
