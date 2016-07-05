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
        if match_field.width <= 32:
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
    #[     int index = *(int*)(value+sizeof(struct ${table.name}_action)); (void)index;
    #[     struct ${table.name}_action* res = (struct ${table.name}_action*)value;
    for counter in table.attached_counters:
        #[ increase_counter(COUNTER_${counter.name}, index);
    #[     if(res == NULL) {
    #[         debug("    :: NO RESULT, NO DEFAULT ACTION, IGNORING PACKET.\n");
    #[         return;
    #[     }
    #[     switch (res->action_id) {
    for action in table.actions:
        #[ case action_${action.name}:
        #[   debug("    :: EXECUTING ACTION ${action.name}...\n");
        if action.signature:
            #[ action_code_${action.name}(pd, tables, res->${action.name}_params);
        else:
            #[ action_code_${action.name}(pd, tables);
        #[     break;
    #[     }
    if 'hit' in table.next_:
        if table.next_['hit'] is not None:
            #[ if(res != NULL)
            #[     apply_table_${table.next_['hit'].name}(pd, tables);
        if table.next_['miss'] is not None:
            #[ if(res == NULL)
            #[     apply_table_${table.next_['miss'].name}(pd, tables);
    else:
        #[ switch (res->action_id) {
        for action, nextnode in table.next_.items():
            #[ case action_${action.name}:
            #[     ${format_p4_node(nextnode)}
            #[     break;
        #[ }
    #[ }
    #[

#[ void init_headers(packet_descriptor_t* packet_desc) {
for hi in header_instances(hlir):
    n = hdr_prefix(hi.name)
    if hi.metadata:
        #[ packet_desc->headers[${n}] = (header_descriptor_t) { .type = ${n}, .length = header_instance_byte_width[${n}],
        #[                               .pointer = calloc(header_instance_byte_width[${n}], sizeof(uint8_t)) };
    else:
        #[ packet_desc->headers[${n}] = (header_descriptor_t) { .type = ${n}, .length = header_instance_byte_width[${n}], .pointer = NULL };
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
#[     init_keyless_tables();
#[ }

#[ 
#[ void handle_packet(packet_descriptor_t* pd, lookup_table_t** tables)
#[ {
#[     int value32;
#[     EXTRACT_INT32_BITS(pd, field_instance_standard_metadata_ingress_port, value32)
#[     debug("### HANDLING PACKET ARRIVING AT PORT %" PRIu32 "...\n", value32);
#[     parse_packet(pd, tables);
#[ }
