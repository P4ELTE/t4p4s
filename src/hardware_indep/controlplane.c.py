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

def match_type_order(t):
    match_types = {
        "exact":    0,
        "lpm":      1,
        "ternary":  2,
    }
    return match_types[t]

#[ #include "dpdk_lib.h"
#[ #include "actions.h"

#[ extern void table_setdefault_promote  (int tableid, uint8_t* value);
#[ extern void exact_add_promote  (int tableid, uint8_t* key, uint8_t* value);
#[ extern void lpm_add_promote    (int tableid, uint8_t* key, uint8_t depth, uint8_t* value);
#[ extern void ternary_add_promote(int tableid, uint8_t* key, uint8_t* mask, uint8_t* value);


for table in hlir16.tables:
    #[ extern void table_${table.name}_key(packet_descriptor_t* pd, uint8_t* key); // defined in dataplane.c


if len(hlir16.tables)>0:
    max_bytes = max([t.key_length_bytes for t in hlir16.tables])
    #[ uint8_t reverse_buffer[$max_bytes];

for table in hlir16.tables:
    #[ // note: ${table.name}, ${table.match_type}, ${table.key_length_bytes}

    params = []
    for key in table.key.keyElements:
        # TODO make an attribute for it
        # key_width_bits = bits_to_bytes(key.width)
        if key.expression.node_type == 'Member':
            if key.get_attr('width') is None:
                # TODO
                continue
            key_width_bits = key.width / 8
            params += "uint8_t {}[{}]".format(key.field_name, key_width_bits),

    #[ void TODO16_${table.name}_add(${', '.join(params)}, struct ${table.name}_action action) {
    #[ }

# Variable width fields are not supported
def get_key_byte_width(k):
    return (k.width+7)/8 if not k.header.type.type_ref.is_vw else 0

for table in hlir16.tables:
    #[ // note: ${table.name}, ${table.match_type}, ${table.key_length_bytes}
    #{ void ${table.name}_add(
    for k in table.key.keyElements:
        byte_width = get_key_byte_width(k)
        #[ uint8_t field_instance_${k.header.name}_${k.field_name}[$byte_width],
        
        # TODO have keys' and tables' match_type the same case (currently: LPM vs lpm)
        if k.match_type == "ternary":
            #[ uint8_t ${k.field_name}_mask[$byte_width],
        if k.match_type == "lpm":
            #[ uint8_t field_instance_${k.header.name}_${k.field_name}_prefix_length,

    #}     struct ${table.name}_action action)
    #{ {

    #[     uint8_t key[${table.key_length_bytes}];

    byte_idx = 0
    for k in sorted(table.key.keyElements, key = lambda k: match_type_order(k.match_type)):
        byte_width = get_key_byte_width(k)
        #[ memcpy(key+$byte_idx, field_instance_${k.header.name}_${k.field_name}, $byte_width);
        byte_idx += byte_width

    if table.match_type == "LPM":
        #[ uint8_t prefix_length = 0;
        for k in table.key.keyElements:
            if table.match_type == "EXACT":
                #[ prefix_length += ${get_key_byte_width(k)};
            if table.match_type == "LPM":
                #[ prefix_length += field_instance_${k.header.name}_${k.field_name}_prefix_length;
        #[ int c, d;
        #[ for(c = ${byte_idx-1}, d = 0; c >= 0; c--, d++) *(reverse_buffer+d) = *(key+c);
        #[ for(c = 0; c < ${byte_idx}; c++) *(key+c) = *(reverse_buffer+c);
        #[ lpm_add_promote(TABLE_${table.name}, (uint8_t*)key, prefix_length, (uint8_t*)&action);

    if table.match_type == "EXACT":
        #[ exact_add_promote(TABLE_${table.name}, (uint8_t*)key, (uint8_t*)&action);

    #} }

for table in hlir16.tables:
    #[ void ${table.name}_setdefault(struct ${table.name}_action action)
    #[ {
    #[     table_setdefault_promote(TABLE_${table.name}, (uint8_t*)&action);
    #[ }


# TODO is there a more appropriate source for this than the annotation?
def get_action_name_str(action):
    return action.action_object.annotations.annotations[0].expr[0].value.replace(".", "")


for table in hlir16.tables:
    #{ void ${table.name}_add_table_entry(struct p4_ctrl_msg* ctrl_m) {
    for i, k in enumerate(table.key.keyElements):
        if k.match_type == "exact":
            #[ uint8_t* field_instance_${k.header.name}_${k.field_name} = (uint8_t*)(((struct p4_field_match_exact*)ctrl_m->field_matches[${i}])->bitmap);
        if k.match_type == "lpm":
            #[ uint8_t* field_instance_${k.header.name}_${k.field_name} = (uint8_t*)(((struct p4_field_match_lpm*)ctrl_m->field_matches[${i}])->bitmap);
            #[ uint16_t field_instance_${k.header.name}_${k.field_name}_prefix_length = ((struct p4_field_match_lpm*)ctrl_m->field_matches[${i}])->prefix_length;

    for action in table.actions:
        # TODO is there a more appropriate source for this than the annotation?
        action_name_str = get_action_name_str(action)
        #{ if(strcmp("$action_name_str", ctrl_m->action_name)==0) {
        #[     struct ${table.name}_action action;
        #[     action.action_id = action_${action.action_object.name};
        for j, p in enumerate(action.action_object.parameters.parameters):
            #[ uint8_t* ${p.name} = (uint8_t*)((struct p4_action_parameter*)ctrl_m->action_params[$j])->bitmap;
            #[ memcpy(action.${action.action_object.name}_params.${p.name}, ${p.name}, ${(p.type.size+7)/8});
        #[     debug("Reply from the control plane arrived.\n");
        #[     debug("Adding new entry to ${table.name} with action ${action.action_object.name}\n");
        #{     ${table.name}_add(
        for i, k in enumerate(table.key.keyElements):
            #[ field_instance_${k.header.name}_${k.field_name},
            if k.match_type == "lpm":
                #[ field_instance_${k.header.name}_${k.field_name}_prefix_length,
        #[     action);
        #}
        #} } else

    #[ debug("Table add entry: action name mismatch (%s).\n", ctrl_m->action_name);
    #} }

for table in hlir16.tables:
    #{ void ${table.name}_set_default_table_action(struct p4_ctrl_msg* ctrl_m) {
    #[ debug("Action name: %s\n", ctrl_m->action_name);
    for action in table.actions:
        action_name_str = get_action_name_str(action)
        #{ if(strcmp("$action_name_str", ctrl_m->action_name)==0) {
        #[     struct ${table.name}_action action;
        #[     action.action_id = action_${action.action_object.name};
        for j, p in enumerate(action.action_object.parameters.parameters):
            #[ uint8_t* ${p.name} = (uint8_t*)((struct p4_action_parameter*)ctrl_m->action_params[$j])->bitmap;
            #[ memcpy(action.${action.action_object.name}_params.${p.name}, ${p.name}, ${(p.type.size+7)/8});
        #[     debug("Message from the control plane arrived.\n");
        #[     debug("Set default action for ${table.name} with action $action_name_str\n");
        #[     ${table.name}_setdefault( action );
        #} } else
    #[ debug("Table setdefault: action name mismatch (%s).\n", ctrl_m->action_name);
    #} }


#[ void recv_from_controller(struct p4_ctrl_msg* ctrl_m) {
#[     debug("MSG from controller %d %s\n", ctrl_m->type, ctrl_m->table_name);
#[     if (ctrl_m->type == P4T_ADD_TABLE_ENTRY) {
for table in hlir16.tables:
    #[ if (strcmp("${table.name}", ctrl_m->table_name) == 0)
    #[     ${table.name}_add_table_entry(ctrl_m);
    #[ else
#[ debug("Table add entry: table name mismatch (%s).\n", ctrl_m->table_name);
#[     }
#[     else if (ctrl_m->type == P4T_SET_DEFAULT_ACTION) {
for table in hlir16.tables:
    #[ if (strcmp("${table.name}", ctrl_m->table_name) == 0)
    #[     ${table.name}_set_default_table_action(ctrl_m);
    #[ else
#[ debug("Table setdefault: table name mismatch (%s).\n", ctrl_m->table_name);
#[     }
#[ }



#[ ctrl_plane_backend bg;
#[ void init_control_plane()
#[ {
#[     debug("Creating control plane connection...\n");
#[     bg = create_backend(3, 1000, "localhost", 11111, recv_from_controller);
#[     launch_backend(bg);
#[ }
