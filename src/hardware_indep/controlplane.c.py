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
from utils.hlir import fld_prefix, fld_id, getTypeAndLength 

def match_type_order(t):
    if t is p4.p4_match_type.P4_MATCH_EXACT:   return 0
    if t is p4.p4_match_type.P4_MATCH_LPM:     return 1
    if t is p4.p4_match_type.P4_MATCH_TERNARY: return 2

#[ #include "dpdk_lib.h"
#[ #include "actions.h"
#[ 
#[ extern void table_setdefault_promote  (int tableid, uint8_t* value);
#[ extern void exact_add_promote  (int tableid, uint8_t* key, uint8_t* value);
#[ extern void lpm_add_promote    (int tableid, uint8_t* key, uint8_t depth, uint8_t* value);
#[ extern void ternary_add_promote(int tableid, uint8_t* key, uint8_t* mask, uint8_t* value);
#[
for table in hlir.p4_tables.values():
    #[ extern void table_${table.name}_key(packet_descriptor_t* pd, uint8_t* key); // defined in dataplane.c
#[

#[ uint8_t reverse_buffer[${max([t[1] for t in map(getTypeAndLength, hlir.p4_tables.values())])}];

for table in hlir.p4_tables.values():
    table_type, key_length = getTypeAndLength(table)
    #[ void
    #[ ${table.name}_add(
    for match_field, match_type, match_mask in table.match_fields:
        byte_width = (match_field.width+7)/8
        #[ uint8_t ${fld_id(match_field)}[${byte_width}],
        ###if match_type is p4_match_type.P4_MATCH_EXACT:
        if match_type is p4.p4_match_type.P4_MATCH_TERNARY:
            #[ uint8_t ${fld_id(match_field)}_mask[${byte_width}],
        if match_type is p4.p4_match_type.P4_MATCH_LPM:
            #[ uint8_t ${fld_id(match_field)}_prefix_length,
    #[ struct ${table.name}_action action)
    #[ {
    #[     uint8_t key[${key_length}];
    sortedfields = sorted(table.match_fields, key=lambda field: match_type_order(field[1]))
    k = 0
    for match_field, match_type, match_mask in sortedfields:
        byte_width = (match_field.width+7)/8
        #[ memcpy(key+${k}, ${fld_id(match_field)}, ${byte_width});
        k += byte_width;
    if table_type == "LOOKUP_LPM":
        #[ uint8_t prefix_length = 0;
        for match_field, match_type, match_mask in table.match_fields:
            byte_width = (match_field.width+7)/8
            if match_type is p4.p4_match_type.P4_MATCH_EXACT:
                #[ prefix_length += ${match_field.width};
            if match_type is p4.p4_match_type.P4_MATCH_LPM:
                #[ prefix_length += ${fld_id(match_field)}_prefix_length;
        #[ int c, d;
        #[ for(c = ${k-1}, d = 0; c >= 0; c--, d++) *(reverse_buffer+d) = *(key+c);
        #[ for(c = 0; c < ${k}; c++) *(key+c) = *(reverse_buffer+c);
        #[ lpm_add_promote(TABLE_${table.name}, (uint8_t*)key, prefix_length, (uint8_t*)&action);
    if table_type == "LOOKUP_EXACT":
        for match_field, match_type, match_mask in table.match_fields:
            byte_width = (match_field.width+7)/8
        #[ exact_add_promote(TABLE_${table.name}, (uint8_t*)key, (uint8_t*)&action);
    #[ }
    #[
    #[ void
    #[ ${table.name}_setdefault(struct ${table.name}_action action)
    #[ {
    #[     table_setdefault_promote(TABLE_${table.name}, (uint8_t*)&action);
    #[ }


for table in hlir.p4_tables.values():
    #[ void
    #[ ${table.name}_add_table_entry(struct p4_ctrl_msg* ctrl_m) {
    for i, m in enumerate(table.match_fields):
        match_field, match_type, match_mask = m
        if match_type is p4.p4_match_type.P4_MATCH_EXACT:
            #[ uint8_t* ${fld_id(match_field)} = (uint8_t*)(((struct p4_field_match_exact*)ctrl_m->field_matches[${i}])->bitmap);
        if match_type is p4.p4_match_type.P4_MATCH_LPM:
            #[ uint8_t* ${fld_id(match_field)} = (uint8_t*)(((struct p4_field_match_lpm*)ctrl_m->field_matches[${i}])->bitmap);
            #[ uint16_t ${fld_id(match_field)}_prefix_length = ((struct p4_field_match_lpm*)ctrl_m->field_matches[${i}])->prefix_length;
    for action in table.actions:
        #[ if(strcmp("${action.name}", ctrl_m->action_name)==0) {
        #[     struct ${table.name}_action action;
        #[     action.action_id = action_${action.name};
        for j, (name, length) in enumerate(zip(action.signature, action.signature_widths)):
            #[ uint8_t* ${name} = (uint8_t*)((struct p4_action_parameter*)ctrl_m->action_params[${j}])->bitmap;
            #[ memcpy(action.${action.name}_params.${name}, ${name}, ${(length+7)/8});
        #[     debug("Reply from the control plane arrived.\n");
        #[     debug("Addig new entry to ${table.name} with action ${action.name}\n");
        #[     ${table.name}_add(
        for m in table.match_fields:
            match_field, match_type, match_mask = m
            #[ ${fld_id(match_field)},
            if match_type is p4.p4_match_type.P4_MATCH_LPM:
                #[ ${fld_id(match_field)}_prefix_length,
        #[     action);
        #[ } else
    #[ debug("Table add entry: action name mismatch (%s).\n", ctrl_m->action_name);
    #[ }

for table in hlir.p4_tables.values():
    #[ void
    #[ ${table.name}_set_default_table_action(struct p4_ctrl_msg* ctrl_m) {
    #[ debug("Action name: %s\n", ctrl_m->action_name);
    for action in table.actions:
        #[ if(strcmp("${action.name}", ctrl_m->action_name)==0) {
        #[     struct ${table.name}_action action;
        #[     action.action_id = action_${action.name};
        for j, (name, length) in enumerate(zip(action.signature, action.signature_widths)):
            #[ uint8_t* ${name} = (uint8_t*)((struct p4_action_parameter*)ctrl_m->action_params[${j}])->bitmap;
            #[ memcpy(action.${action.name}_params.${name}, ${name}, ${(length+7)/8});
        #[     debug("Message from the control plane arrived.\n");
        #[     debug("Set default action for ${table.name} with action ${action.name}\n");
        #[     ${table.name}_setdefault( action );
        #[ } else
    #[ debug("Table setdefault: action name mismatch (%s).\n", ctrl_m->action_name);
    #[ }


#[ void recv_from_controller(struct p4_ctrl_msg* ctrl_m) {
#[     debug("MSG from controller %d %s\n", ctrl_m->type, ctrl_m->table_name);
#[     if (ctrl_m->type == P4T_ADD_TABLE_ENTRY) {
for table in hlir.p4_tables.values():
    #[ if (strcmp("${table.name}", ctrl_m->table_name) == 0)
    #[     ${table.name}_add_table_entry(ctrl_m);
    #[ else
#[ debug("Table add entry: table name mismatch (%s).\n", ctrl_m->table_name);
#[     }
#[     else if (ctrl_m->type == P4T_SET_DEFAULT_ACTION) {
for table in hlir.p4_tables.values():
    #[ if (strcmp("${table.name}", ctrl_m->table_name) == 0)
    #[     ${table.name}_set_default_table_action(ctrl_m);
    #[ else
#[ debug("Table setdefault: table name mismatch (%s).\n", ctrl_m->table_name);
#[     }
#[ }



#[ backend bg;
#[ void init_control_plane()
#[ {
#[     debug("Creating control plane connection...\n");
#[     bg = create_backend(3, 1000, "localhost", 11111, recv_from_controller);
#[     launch_backend(bg);
#[ /*
if "smac" in hlir.p4_tables:
    #[ struct smac_action action;
    #[ action.action_id = action_mac_learn;
    #[ smac_setdefault(action);
    #[ debug("smac setdefault\n");
if "dmac" in hlir.p4_tables:
    #[ struct dmac_action action2;
    #[ action2.action_id = action_bcast;
    #[ dmac_setdefault(action2);
    #[ debug("dmac setdefault\n");
#[ */
#[ }
