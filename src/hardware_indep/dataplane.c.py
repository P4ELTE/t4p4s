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
#[ #include "dpdk_lib.h"
#[ #include "actions.h"
#[ 
#[ extern void parse_packet(packet_descriptor_t* pd, lookup_table_t** tables);
#[
#[ extern void increase_counter (int counterid, int index);
#[
for table in hlir.p4_tables.values():
    #[ void apply_table_${table.name}(packet_descriptor_t* pd, lookup_table_t** tables);
#[

if len(hlir.p4_tables.values())>0:
    #[ uint8_t reverse_buffer[${max([t[1] for t in map(getTypeAndLength, hlir.p4_tables.values())])}];

def match_type_order(t):
    if t is p4.p4_match_type.P4_MATCH_EXACT:   return 0
    if t is p4.p4_match_type.P4_MATCH_LPM:     return 1
    if t is p4.p4_match_type.P4_MATCH_TERNARY: return 2

for table in hlir.p4_tables.values():
    table_type, key_length = getTypeAndLength(table)
    #[ void table_${table.name}_key(packet_descriptor_t* pd, uint8_t* key) {
    sortedfields = sorted(table.match_fields, key=lambda field: match_type_order(field[1]))
    for match_field, match_type, match_mask in sortedfields:
        if is_vwf(match_field):
            addError("generating table_" + table.name + "_key", "Variable width field '" + str(match_field) + "' in match key for table '" + table.name + "' is not supported")
        elif match_field.width <= 32:
            #[ EXTRACT_INT32_BITS(pd, ${fld_id(match_field)}, *(uint32_t*)key)
            #[ key += sizeof(uint32_t);
        elif match_field.width > 32 and match_field.width % 8 == 0:
            byte_width = (match_field.width+7)/8
            #[ EXTRACT_BYTEBUF(pd, ${fld_id(match_field)}, key)
            #[ key += ${byte_width};
        else:
            print "Unsupported field %s ignored in key calculation." % fld_id(match_field)
    if table_type == "LOOKUP_LPM":
        #[ key -= ${key_length};
        #[ int c, d;
        #[ for(c = ${key_length-1}, d = 0; c >= 0; c--, d++) *(reverse_buffer+d) = *(key+c);
        #[ for(c = 0; c < ${key_length}; c++) *(key+c) = *(reverse_buffer+c);
    #[ }
    #[

for table in hlir.p4_tables.values():
    table_type, key_length = getTypeAndLength(table)
    lookupfun = {'LOOKUP_LPM':'lpm_lookup', 'LOOKUP_EXACT':'exact_lookup', 'LOOKUP_TERNARY':'ternary_lookup'}
    #[ void apply_table_${table.name}(packet_descriptor_t* pd, lookup_table_t** tables)
    #[ {
    #[     debug("  :::: EXECUTING TABLE ${table.name}\n");
    #[     uint8_t* key[${key_length}];
    #[     table_${table.name}_key(pd, (uint8_t*)key);
    #[     uint8_t* value = ${lookupfun[table_type]}(tables[TABLE_${table.name}], (uint8_t*)key);
    #[     struct ${table.name}_action* res = (struct ${table.name}_action*)value;
    #[     int index; (void)index;

    # COUNTERS
    #[     if(res != NULL) {
    #[       index = *(int*)(value+sizeof(struct ${table.name}_action));
    for counter in table.attached_counters:
        #[       increase_counter(COUNTER_${counter.name}, index);
    #[     }

    # ACTIONS
    #[     if(res == NULL) {
    #[       debug("    :: NO RESULT, NO DEFAULT ACTION.\n");
    #[     } else {
    #[       switch (res->action_id) {
    for action in table.actions:
        #[         case action_${action.name}:
        #[           debug("    :: EXECUTING ACTION ${action.name}...\n");
        if action.signature:
            #[           action_code_${action.name}(pd, tables, res->${action.name}_params);
        else:
            #[           action_code_${action.name}(pd, tables);
        #[           break;
    #[       }
    #[     }

    # NEXT TABLE
    if 'hit' in table.next_:
        #[     if(res != NULL && index != DEFAULT_ACTION_INDEX) { //Lookup was successful (not with default action)
        if table.next_['hit'] is not None:
            #[       ${format_p4_node(table.next_['hit'])}
        #[     } else {                                           //Lookup failed or returned default action
        if table.next_['miss'] is not None:
            #[       ${format_p4_node(table.next_['miss'])}
        #[     }
    else:
        #[     if (res != NULL) {
        #[       switch (res->action_id) {
        for action, nextnode in table.next_.items():
            #[         case action_${action.name}:
            #[           ${format_p4_node(nextnode)}
            #[           break;
        #[       }
        #[     } else {
        #[       debug("    :: IGNORING PACKET.\n");
        #[       return;
        #[     }
    #[ }
    #[

#[
#[ uint16_t csum16_add(uint16_t num1, uint16_t num2) {
#[     if(num1 == 0) return num2;
#[     uint32_t tmp_num = num1 + num2;
#[     while(tmp_num > 0xffff)
#[         tmp_num = ((tmp_num & 0xffff0000) >> 16) + (tmp_num & 0xffff);
#[     return (uint16_t)tmp_num;
#[ }
#[
for calc in hlir.p4_field_list_calculations.values():
    #[ uint32_t calculate_${calc.name}(packet_descriptor_t* pd) {
    #[   uint32_t res = 0;
    #[   void* payload_ptr;

    buff_idx = 0
    fixed_input_width = 0     #Calculates the fixed width of all p4_fields and sized_integers (PAYLOAD width is not included)
    variable_input_width = "" #Calculates the variable width of all p4_fields
    for field_list in calc.input:
        for item in field_list.fields:
            if isinstance(item, p4_field) or isinstance(item, p4_sized_integer):
                if is_vwf(item):
                    if field_max_width(item) % 8 == 0 and item.offset % 8 == 0:
                        variable_input_width += " + field_desc(pd, " + fld_id(item) + ").bitwidth"
                    else:
                        addError("generating field list calculation " + calc.name, "Variable width field '" + str(item) + "' in calculation '" + calc.name + "' is not byte-aligned. Field list calculations are only supported on byte-aligned variable width fields!");
                else:
                    fixed_input_width += item.width
    if fixed_input_width % 8 != 0:
        addError("generating field list calculation", "The bitwidth of the field_lists for the calculation '" + calc.name + "' is incorrect.")
    #[   uint8_t* buf = malloc((${fixed_input_width}${variable_input_width}) / 8);
    #[   memset(buf, 0, (${fixed_input_width}${variable_input_width}) / 8);

    tmp_list = []
    fixed_bitoffset = 0
    variable_bitoffset = ""
    for field_list in calc.input:
        item_index = 0
        while item_index < len(field_list.fields):
            start_item = field_list.fields[item_index]
            if isinstance(start_item, p4_field): #Processing field block (multiple continuous fields in a row)
                inst = start_item.instance
                if is_vwf(start_item):
                    fixed_bitwidth = 0
                    variable_bitwidth = " + field_desc(pd, " + fld_id(start_item) + ").bitwidth"
                else:
                    fixed_bitwidth = start_item.width
                    variable_bitwidth = ""

                inst_index = 0 #The index of the field in the header instance
                while start_item != inst.fields[inst_index]: inst_index += 1

                while inst_index + 1 < len(inst.fields) and item_index + 1 < len(field_list.fields) and inst.fields[inst_index + 1] == field_list.fields[item_index + 1]:
                    item_index += 1
                    inst_index += 1
                    if is_vwf(field_list.fields[item_index]):
                        variable_bitwidth += " + field_desc(pd, " + fld_id(field_list.fields[item_index]) + ").bitwidth"
                    else:
                        fixed_bitwidth += field_list.fields[item_index].width

                if (not variable_bitwidth) and fixed_bitwidth % 8 != 0: addError("generating field list calculation", "The bitwidth of a field block is incorrenct!")
                tmp_list.append(("((" + str(fixed_bitoffset) + variable_bitoffset + ") / 8)", start_item, "((" + str(fixed_bitwidth) + variable_bitwidth + ") / 8)"))

                fixed_bitoffset += fixed_bitwidth
                variable_bitoffset += variable_bitwidth
            elif isinstance(start_item, p4_sized_integer):
                if start_item.width % 8 != 0:
                    addError("generating field list calculation", "Only byte-wide constants are supported in field lists.")
                else:
                    buff_idx += 1
                    byte_array = int_to_big_endian_byte_array_with_length(start_item, start_item.width / 8)
                    #[   uint8_t buffer_${buff_idx}[${start_item.width / 8}] = {${reduce((lambda a, b: a + ', ' + b), map(lambda x: str(x), byte_array))}};
                    #[   memcpy(buf + ((${fixed_bitoffset}${variable_bitoffset}) / 8), &buffer_${buff_idx}, ${start_item.width / 8});
                    fixed_bitoffset += start_item.width
            else:
                if item_index == 0 or not isinstance(field_list.fields[item_index - 1], p4_field):
                    addError("generating field list calculation", "Payload element must follow a regular field instance in the field list.")
                elif calc.algorithm == "csum16":
                    hi_name = hdr_prefix(field_list.fields[item_index - 1].instance.name)
                    #[   payload_ptr = (((void*)pd->headers[${hi_name}].pointer) + (pd->headers[${hi_name}].length));
                    #[   res = csum16_add(res, calculate_csum16(payload_ptr, packet_length(pd) - (payload_ptr - ((void*) pd->data))));
            item_index += 1

    while len(tmp_list) > 0:
        inst = tmp_list[0][1].instance
        list_index = 0
        #[   if(${format_expr(valid_expression(inst))}) {
        while list_index < len(tmp_list):
            item = tmp_list[list_index]
            if item[1].instance == inst:
                #[     memcpy(buf + ${item[0]}, field_desc(pd, ${fld_id(item[1])}).byte_addr, ${item[2]});
                del tmp_list[list_index]
            else: list_index += 1;
        #[   }

    if calc.algorithm == "csum16":
        #[   res = csum16_add(res, calculate_csum16(buf, (${fixed_input_width}${variable_input_width}) / 8));
        #[   res = (res == 0xffff) ? res : ((~res) & 0xffff);
    else:
        #If a new calculation implementation is added, new PAYLOAD handling should also be added.
        addError("generating field list calculation", "Unsupported field list calculation algorithm: " + calc.algorithm)
    #[   free(buf);
    #[   return res & ${hex((2 ** calc.output_width) - 1)};
    #[ }
    #[

#[ void reset_headers(packet_descriptor_t* packet_desc) {
for hi in header_instances(hlir):
    n = hdr_prefix(hi.name)
    if hi.metadata:
        #[ memset(packet_desc->headers[${n}].pointer, 0, header_info(${n}).bytewidth * sizeof(uint8_t));
    else:
        #[ packet_desc->headers[${n}].pointer = NULL;
#[ }
#[ void init_headers(packet_descriptor_t* packet_desc) {
for hi in header_instances(hlir):
    n = hdr_prefix(hi.name)
    if hi.metadata:
        #[ packet_desc->headers[${n}] = (header_descriptor_t) { .type = ${n}, .length = header_info(${n}).bytewidth,
        #[                               .pointer = malloc(header_info(${n}).bytewidth * sizeof(uint8_t)),
        #[                               .var_width_field_bitwidth = 0 };
    else:
        #[ packet_desc->headers[${n}] = (header_descriptor_t) { .type = ${n}, .length = header_info(${n}).bytewidth, .pointer = NULL,
        #[                               .var_width_field_bitwidth = 0 };
#[ }
#[
for table in hlir.p4_tables.values():
    table_type, key_length = getTypeAndLength(table)
    if key_length == 0 and len(table.actions) == 1:
        action = table.actions[0]
        #[ extern void ${table.name}_setdefault(struct ${table.name}_action);
#[
#[ void init_keyless_tables() {
for table in hlir.p4_tables.values():
    table_type, key_length = getTypeAndLength(table)
    if key_length == 0 and len(table.actions) == 1:
        action = table.actions[0]
        #[ struct ${table.name}_action ${table.name}_a;
        #[ ${table.name}_a.action_id = action_${action.name};
        #[ ${table.name}_setdefault(${table.name}_a);
#[ }
#[
#[ void init_dataplane(packet_descriptor_t* pd, lookup_table_t** tables) {
#[     init_headers(pd);
#[     reset_headers(pd);
#[     init_keyless_tables();
#[     pd->dropped=0;
#[ }

#[
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
#[
for f in hlir.p4_fields.values():
    for calc in f.calculation:
        if calc[0] == "update":
            if calc[2] is not None:
                #[ if(${format_expr(calc[2])})
            elif not f.instance.metadata:
                #[ if(${format_expr(valid_expression(f))})
            #[ {
            #[     value32 = calculate_${calc[1].name}(pd);
            #[     MODIFY_INT32_INT32_BITS(pd, ${fld_id(f)}, value32);
            #[ }
#[ }
#[

#[
#[ int verify_packet(packet_descriptor_t* pd) {
#[   uint32_t value32;
for f in hlir.p4_fields.values():
    for calc in f.calculation:
        if calc[0] == "verify":
            if calc[2] is not None:
                #[   if(${format_expr(calc[2])})
            elif not f.instance.metadata:
                #[   if(${format_expr(valid_expression(f))})
            #[   {
            #[     EXTRACT_INT32_BITS(pd, ${fld_id(f)}, value32);
            #[     if(value32 != calculate_${calc[1].name}(pd)) {
            #[       debug("       Checksum verification on field '${f}' by '${calc[1].name}': FAILED\n");
            #[       return 1;
            #[     }
            #[     else debug("       Checksum verification on field '${f}' by '${calc[1].name}': SUCCESSFUL\n");
            #[   }
#[   return 0;
#[ }
#[

#[ 
#[ void handle_packet(packet_descriptor_t* pd, lookup_table_t** tables)
#[ {
#[     int value32;
#[     EXTRACT_INT32_BITS(pd, field_instance_standard_metadata_ingress_port, value32)
#[     debug("### HANDLING PACKET ARRIVING AT PORT %" PRIu32 "...\n", value32);
#[     parse_packet(pd, tables);
#[     update_packet(pd);
#[ }
