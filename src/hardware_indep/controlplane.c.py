# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from compiler_common import unique_everseen
from utils.codegen import format_expr, format_type

#[ #include <unistd.h>

#[ #include "dpdk_lib.h"
#[ #include "actions.h"
#[ #include "tables.h"

#{ #ifdef T4P4S_P4RT
#[     #include "PI/proto/pi_server.h"
#[     #include "p4rt/device_mgr.h"
#[     extern device_mgr_t *dev_mgr_ptr;
#} #endif


for table in hlir.tables:
    #[ extern void table_${table.name}_key(packet_descriptor_t* pd, uint8_t* key); // defined in dataplane.c


max_bytes = max([0] + [t.key_length_bytes for t in hlir.tables])
#[ uint8_t reverse_buffer[$max_bytes];

# Variable width fields are not supported
def get_key_byte_width(k):
    if 'size' in k:
        return (k.size+7)//8

    if k.header.urtype('is_vw', False):
        return 0

    return (k.header.type.size+7)//8


for table in hlir.tables:
    #[ // note: ${table.name}, ${table.matchType.name}, ${table.key_length_bytes}
    #{ void ${table.name}_add(
    for k in table.key.keyElements:
        if 'header' not in k:
            varname = k.expression.path.name
            #[ uint8_t* $varname,
            # ${format_type(k.expression.urtype)} $varname,
            # TODO mask?
        else:
            byte_width = get_key_byte_width(k)
            #[ uint8_t field_${k.header.name}_${k.field_name}[$byte_width],
            # TODO have keys' and tables' matchType the same case (currently: LPM vs lpm)
            if k.matchType.path.name == "ternary": # TODO: LS Check!
                #[ uint8_t ${k.field_name}_mask[$byte_width],
            if k.matchType.path.name == "lpm": # TODO: LS Check!
                #[ uint8_t field_${k.header.name}_${k.field_name}_prefix_length,

    #}     ${table.name}_action_t action, bool has_fields)
    #{ {
    #[     uint8_t key[${table.key_length_bytes}];

    byte_idx = 0
    for k in sorted((k for k in table.key.keyElements), key = lambda k: k.match_order):
        byte_width = get_key_byte_width(k)
        target_name = f'{k.expression.path.name}' if 'header' not in k else f'field_{k.header.name}_{k.field_name}'
        #[ memcpy(key+$byte_idx, (uint8_t*)${target_name}, $byte_width);
        byte_idx += byte_width

    if table.matchType.name == "lpm":
        #[ uint8_t prefix_length = 0;
        for k in table.key.keyElements:
            target_name = f'{k.expression.path.name}' if 'header' not in k else f'field_{k.header.name}_{k.field_name}'
            if k.matchType.path.name == "exact": # TODO: LS Check!
                #[ prefix_length += ${get_key_byte_width(k)};
            if k.matchType.path.name == "lpm": # TODO: LS Check!
                #[ prefix_length += ${target_name}_prefix_length;
        #[ int c, d;
        #[ for(c = ${byte_idx-1}, d = 0; c >= 0; c--, d++) *(reverse_buffer+d) = *(key+c);
        #[ for(c = 0; c < ${byte_idx}; c++) *(key+c) = *(reverse_buffer+c);
        #[ lpm_add_promote(TABLE_${table.name}, (uint8_t*)key, prefix_length, (uint8_t*)&action, false, has_fields);

    if table.matchType.name == "exact":
        #[ exact_add_promote(TABLE_${table.name}, (uint8_t*)key, (uint8_t*)&action, false, has_fields);

    #} }

for table in hlir.tables:
    #{ void ${table.name}_add_table_entry(struct p4_ctrl_msg* ctrl_m) {
    for i, k in enumerate(table.key.keyElements):
        target_name = f'{k.expression.path.name}' if 'header' not in k else f'field_{k.header.name}_{k.field_name}'

        if k.matchType.path.name == "exact":
            #[ uint8_t* ${target_name} = (uint8_t*)(((struct p4_field_match_exact*)ctrl_m->field_matches[${i}])->bitmap);
        if k.matchType.path.name == "lpm":
            #[ uint8_t* ${target_name} = (uint8_t*)(((struct p4_field_match_lpm*)ctrl_m->field_matches[${i}])->bitmap);
            #[ uint16_t ${target_name}_prefix_length = ((struct p4_field_match_lpm*)ctrl_m->field_matches[${i}])->prefix_length;
        if k.matchType.path.name == "ternary":
            # TODO are these right?
            #[ uint8_t* ${target_name} = (uint8_t*)(((struct p4_field_match_lpm*)ctrl_m->field_matches[${i}])->bitmap);
            #[ uint16_t ${target_name}_prefix_length = ((struct p4_field_match_lpm*)ctrl_m->field_matches[${i}])->prefix_length;

    for action in table.actions:
        #{ if(strcmp("${action.action_object.canonical_name}", ctrl_m->action_name)==0) {
        #[     ${table.name}_action_t action;
        #[     action.action_id = action_${action.action_object.name};
        for j, p in enumerate(action.action_object.parameters.parameters):
            #[ uint8_t* bitmap_${p.name} = (uint8_t*)((struct p4_action_parameter*)ctrl_m->action_params[$j])->bitmap;
            #[ memcpy(action.${action.action_object.name}_params.${p.name}, bitmap_${p.name}, ${(p.urtype.size+7)//8});

        params = []
        for i, k in enumerate(table.key.keyElements):
            target_name = f'{k.expression.path.name}' if 'header' not in k else f'field_{k.header.name}_{k.field_name}'

            params.append(target_name)
            if k.matchType.path.name == "lpm":
                params.append(f"{target_name}_prefix_length")
            if k.matchType.path.name == "ternary":
                params.append("0 /* TODO ternary dstPort_mask */")

        has_fields = "false" if len(action.action_object.parameters.parameters) == 0 else "true"
        joined_params = ', '.join(params + ["action", has_fields])
        #[ ${table.name}_add($joined_params);

        for j, p in enumerate(action.action_object.parameters.parameters):
            size = p.urtype.size

        #} } else

    valid_actions = ", ".join([f'" T4LIT({a.action_object.canonical_name},action) "' for a in table.actions])
    #[     debug(" $$[warning]{}{!!!! Table add entry} on table $$[table]{table.canonical_name}: action name $$[warning]{}{mismatch}: $$[action]{}{%s}, expected one of ($valid_actions).\n", ctrl_m->action_name);
    #} }
    #[

for table in hlir.tables:
    #{ void ${table.name}_set_default_table_action(struct p4_ctrl_msg* ctrl_m) {
    for action in table.actions:
        #{ if(strcmp("${action.action_object.canonical_name}", ctrl_m->action_name)==0) {
        #[     ${table.name}_action_t action;
        #[     action.action_id = action_${action.action_object.name};

        for j, p in enumerate(action.action_object.parameters.parameters):
            #[ uint8_t* ${p.name} = (uint8_t*)((struct p4_action_parameter*)ctrl_m->action_params[$j])->bitmap;
            #[ memcpy(action.${action.action_object.name}_params.${p.name}, ${p.name}, ${(p.urtype.size+7)//8});

        #{     if (${"false" if table.is_hidden else "true"}) {
        #[         debug(" " T4LIT(ctl>,incoming) " " T4LIT(Set default action,action) " for $$[table]{table.canonical_name}: $$[action]{action.action_object.canonical_name}\n");
        #}     }
        #[     table_setdefault_promote(TABLE_${table.name}, (uint8_t*)&action);
        #} } else

    valid_actions = ", ".join([f'" T4LIT({a.action_object.canonical_name},action) "' for a in table.actions])
    #[ debug(" $$[warning]{}{!!!! Table setdefault} on table $$[table]{table.name}: action name $$[warning]{}{mismatch} ($$[action]{}{%s}), expected one of ($valid_actions)\n", ctrl_m->action_name);
    #} }
    #[


all_keyed_table_names = ", ".join((f'"T4LIT({table.canonical_name},table)"' for table in hlir.tables))
common_keyed_table_names = ", ".join((f'"T4LIT({table.canonical_name},table)"' for table in hlir.tables.filterfalse('is_hidden')))
hidden_table_count = len(hlir.tables.filter('is_hidden'))

#{ #ifdef T4P4S_DEBUG
#[     bool possible_tables_already_shown = false;
#{     #ifdef T4P4S_SHOW_HIDDEN_TABLES
#[         bool show_hidden_tables = true;
#[     #else
#[         bool show_hidden_tables = false;
#}     #endif

#{     void debug_show_possible_tables() {
#[         if (possible_tables_already_shown)   return;
#{         if (show_hidden_tables) {
#[             debug("   !! Possible table names: $all_keyed_table_names\n");
#[         } else {
#[             debug("   !! Possible table names: $common_keyed_table_names and " T4LIT(%d) " hidden tables\n", $hidden_table_count);
#}         }
#[         possible_tables_already_shown = true;
#}     }

#} #endif

#{ void ctrl_add_table_entry(struct p4_ctrl_msg* ctrl_m) {
for table in hlir.tables:
    #{ if (strcmp("${table.canonical_name}", ctrl_m->table_name) == 0) {
    #[     ${table.name}_add_table_entry(ctrl_m);
    #[     return;
    #} }
#[     debug(" $$[warning]{}{!!!! Table add entry}: $$[warning]{}{unknown table name} $$[table]{}{%s}\n", ctrl_m->table_name);
#{     #ifdef T4P4S_DEBUG
#[         debug_show_possible_tables();
#}     #endif
#} }


#[ extern char* action_names[];

#{ void ctrl_setdefault(struct p4_ctrl_msg* ctrl_m) {
for table in hlir.tables:
    #{ if (strcmp("${table.canonical_name}", ctrl_m->table_name) == 0) {
    #[     ${table.name}_set_default_table_action(ctrl_m);
    #[     return;
    #} }

#[     debug(" $$[warning]{}{!!!! Table set default}: $$[warning]{}{unknown table name} $$[table]{}{%s}\n", ctrl_m->table_name);
#{     #ifdef T4P4S_DEBUG
#[         debug_show_possible_tables();
#}     #endif
#} }

hack_i = 0
for smem in unique_everseen([smem for table, smem in hlir.all_counters]):
    for target in smem.smem_for:
        if not smem.smem_for[target]:
            continue
        hack_i += 1
        if hack_i%2==1:
            for c in smem.components:
                cname = c['name']
                if smem.smem_type not in ["register", "direct_counter", "direct_meter"]:
                    #[ uint32_t ctrl_${cname}[${smem.amount}];

#{ uint32_t* read_counter_value_by_name(char* counter_name, int* size, bool is_bytes){
#[ int i;
hack_i = 0
for smem in unique_everseen([smem for table, smem in hlir.all_counters]):
    for target in smem.smem_for:
        if not smem.smem_for[target]:
            continue
        hack_i += 1
        if hack_i%2==0:
            continue

        for c in smem.components:
            cname = c['name']
            pre_bytes = ''
            if c['for'] == "packets":
                pre_bytes = '!'
            if smem.smem_type not in ["register", "direct_counter", "direct_meter"]:
                #{ if ((strcmp("${smem.canonical_name}", counter_name) == 0) && (${pre_bytes}is_bytes)) {
                #[   *size = ${smem.amount};
                #[   for (i=0;i<${smem.amount};++i) ctrl_${cname}[i] = global_smem.${cname}[i].value.cnt;
                #[   return ctrl_${cname};
                #} }
#[   *size = -1;
#[   return 0;
#} }



#[ extern struct socket_state state[NB_SOCKETS];

#[ extern volatile int ctrl_is_initialized;

#{ void ctrl_initialized() {
#[     debug("   " T4LIT(::,incoming) " Control plane init " T4LIT(done,success) "\n");
#[     ctrl_is_initialized = 1;
#} }


#{ void recv_from_controller(struct p4_ctrl_msg* ctrl_m) {
#{     if (ctrl_m->type == P4T_ADD_TABLE_ENTRY) {
#[         ctrl_add_table_entry(ctrl_m);
#[     } else if (ctrl_m->type == P4T_SET_DEFAULT_ACTION) {
#[         ctrl_setdefault(ctrl_m);
#[     } else if (ctrl_m->type == P4T_CTRL_INITIALIZED) {
#[         ctrl_initialized();
#[     } else if (ctrl_m->type == P4T_READ_COUNTER) {
#[         //ctrl_m->xid = *read_counter_value_by_name(ctrl_m->table_name);
#{         //TODO:SEND BACK;
#}     }
#} }


#[ ctrl_plane_backend bg;

#{ #ifdef T4P4S_P4RT
#[     void init_control_plane()
#{     {
#[         bg = create_backend(3, 1000, "localhost", 11111, recv_from_controller);
#[         launch_backend(bg);
#[         dev_mgr_init_with_t4p4s(dev_mgr_ptr, recv_from_controller, read_counter_value_by_name, 1);
#[         PIGrpcServerRunAddrGnmi("127.0.0.1:50051", 0);
#[         PIGrpcServerRun();
#}     }
#[ #else
#[     void init_control_plane()
#{     {
#[         bg = create_backend(3, 1000, "localhost", 11111, recv_from_controller);
#[         launch_backend(bg);
#[
#{         #ifdef T4P4S_DEBUG
#{         for (int i = 0; i < NB_TABLES; i++) {
#[             lookup_table_t t = table_config[i];
#[             if (state[0].tables[t.id][0]->init_entry_count > 0)
#[                 debug("    " T4LIT(:,incoming) " Table " T4LIT(%s,table) " got " T4LIT(%d) " entries from the control plane\n", state[0].tables[t.id][0]->canonical_name, state[0].tables[t.id][0]->init_entry_count);
#}             }
#{         #endif
#}     }
#} #endif

