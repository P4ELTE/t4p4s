# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from hlir16.hlir_utils import align8_16_32
from compiler_common import unique_everseen, generate_var_name, get_hdr_name, get_hdrfld_name
from utils.codegen import format_expr, format_type, gen_format_slice
from utils.extern import get_smem_name

import os

#[ #include <unistd.h>

#[ #include "dpdk_lib.h"
#[ #include "dpdk_primitives.h" // TODO remove
#[ #include "actions.h"
#[ #include "tables.h"
#[ #include "controlplane.h"

#{ #ifdef T4P4S_P4RT
#[     #include "PI/proto/pi_server.h"
#[     #include "p4rt/device_mgr.h"
#[     extern device_mgr_t *dev_mgr_ptr;
#} #endif


for table in hlir.tables:
    if len(table.key.keyElements) > 0:
        #[ extern void table_${table.name}_key(packet_descriptor_t* pd, uint8_t* key); // defined in dataplane.c


# Variable width fields are not supported
def get_key_byte_width(k):
    if 'size' in k:
        return (k.size+7)//8

    if k.header.urtype('is_vw', False):
        return 0

    return (k.header.type.size+7)//8



def slice_info(e):
    hdrname, fldname = get_hdrfld_name(e.e0)
    maxval  = e.e1.value
    minval  = e.e1.value
    return hdrname, fldname, maxval, minval


def get_key_name_postfix(k, idx):
    if 'header_name' in k:
        return f'_{k.header_name}_{k.field_name}'

    ke = k.expression

    if ke.node_type == 'Slice':
        hdrname, fldname, maxval, minval = slice_info(ke)
        return f'_{hdrname}_{fldname}_{maxval}_{minval}'

    return ''

def get_key_name(k, idx):
    return f'keyelem_{idx}_{k.node_type}{get_key_name_postfix(k, idx)}'

for table in hlir.tables:
    if len(table.key.keyElements) > 0:
        #{ typedef struct {
        for idx, k in enumerate(sorted(table.key.keyElements, key = lambda k: k.match_order)):
            varname = get_key_name(k, idx)
            #[     ${format_type(k.expression.type, varname)};
        #} } table_key_${table.name}_t;
        #[


# TODO move to hlir_attrib
for table in hlir.tables:
    for k in table.key.keyElements:
        if 'header' in k:
            k.src = f'field_{k.header.name}_{k.field_name}'
            continue

        ke = k.expression
        # TODO implement these two
        if ke.node_type == 'MethodCallExpression':
            k.src = None
        elif ke.node_type == 'Slice':
            k.src = None
        else:
            k.src = f'{ke.path.name}'


def gen_key_component_param(k, byte_width):
    if 'header' in k:
        #[     uint8_t field_${k.header.name}_${k.field_name}[$byte_width],
    else:
        varname = generate_var_name(f'target_{k.src}')
        #[     uint8_t* $varname,

    if k.src is not None:
        # TODO it should never be None

        if k.matchType.path.name == "ternary":
            #[     uint8_t ${k.src}_mask[$byte_width],
        if k.matchType.path.name == "lpm":
            #[     uint8_t ${k.src}_prefix_length,


def gen_fill_key_component_slice(ke):
    src = generate_var_name('slice')

    slice_expr = gen_format_slice(ke)

    #= compiler_common.pre_statement_buffer
    #[ ${format_type(ke.e0.type)} $src = ${slice_expr};
    #= compiler_common.post_statement_buffer

    compiler_common.pre_statement_buffer = ""
    compiler_common.post_statement_buffer = ""


def gen_fill_key_component(k, idx, byte_width, tmt, kmt):
    ke = k.expression
    if ke.node_type == 'MethodCallExpression':
        # TODO can this be anything other than a call to isValid?
        target = generate_var_name('method_call')

        value = 1;
        #[     ${format_type(ke.type, target)} = $value;
    elif ke.node_type == 'Slice':
        #[     // TODO fill Slice component properly (call gen_fill_key_component_slice)
    else:
        to_t4p4s_order = byte_width == 4 and ('header' in k and not k.header.urtype.is_metadata)
        if to_t4p4s_order:
            padded_byte_width = align8_16_32(8*byte_width)
            varname = generate_var_name(f'fld_{get_key_name(k, idx)}')
            #[     uint${padded_byte_width}_t $varname = *(uint${padded_byte_width}_t*)field_matches[$idx]->bitmap;
            #[     key->${get_key_name(k, idx)} = net2t4p4s_${int(padded_byte_width/8)}($varname);
        else:
            #[     memcpy(&(key->${get_key_name(k, idx)}), field_matches[$idx]->bitmap, $byte_width);

        if tmt == "lpm":
            if kmt == "exact":
                #[     prefix_length += ${get_key_byte_width(k)};
            if kmt == "lpm":
                #[     prefix_length += ${byte_width};


for table in hlir.tables:
    tmt = table.matchType.name

    return_t     = {'exact': 'void', 'lpm': 'uint8_t', 'ternary': 'void'}
    extra_init   = {'exact': '', 'lpm': 'uint8_t prefix_length = 0;', 'ternary': ''}
    extra_return = {'exact': '', 'lpm': 'return prefix_length;', 'ternary': ''}

    if len(table.key.keyElements) > 0:
        #[ // note: ${table.short_name} alias ${table.canonical_name} alias ${table.name}, $tmt, ${table.key_length_bytes}
        #{ ${return_t[tmt]} ${table.name}_setup_key(p4_field_match_${tmt}_t** field_matches, table_key_${table.name}_t* key) {
        if extra_init[tmt]:
            #[     ${extra_init[tmt]}

        for i, k in enumerate(sorted(table.key.keyElements, key = lambda k: k.match_order)):
            kmt = k.matchType.path.name

            if kmt == "lpm":
                #[     prefix_length += field_matches[$i]->prefix_length;
            if kmt == "ternary":
                #[     /* TODO ternary */


        for idx, k in enumerate(sorted(table.key.keyElements, key = lambda k: k.match_order)):
            byte_width = get_key_byte_width(k)
            #= gen_fill_key_component(k, idx, byte_width, tmt, kmt)

        if extra_return[tmt]:
            #[     ${extra_return[tmt]}

        #} }
        #[


for table in hlir.tables:
    # with open(os.path.join(t4p4sdir, 'output_translate.txt'), 'w') as outf:
    #     for action in table.actions:
    #         if action.action_object.name == f'({action.action_object.canonical_name})':
    #             continue
    #         print(f"TRANSLATE {action.action_object.name} {action.action_object.canonical_name}")
    #         outf.write(f"TRANSLATE {action.action_object.name} {action.action_object.canonical_name}")
    pass


for table in hlir.tables:
    #{ bool ${table.name}_setup_entry(ENTRY(${table.name})* entry, p4_action_parameter_t** action_params, const char* action_name) {
    for idx, action in enumerate(table.actions):
        ao = action.action_object
        if idx == 0:
            #{     if (strcmp("${ao.canonical_name}", action_name)==0) {
        else:
            #[     } else if (strcmp("${ao.canonical_name}", action_name)==0) {

        #[         entry->id = action_${ao.name};
        for pidx, p in enumerate(ao.parameters.parameters):
            #[         memcpy(&entry->params.${ao.name}_params.${p.name}, action_params[$pidx]->bitmap, ${(p.urtype.size+7)//8});

    valid_actions = ", ".join(f'" T4LIT({a.action_object.canonical_name},action) "' for a in table.actions)
    #[     } else {
    #[         debug(" $$[warning]{}{!!!! Table add entry} on table " T4LIT(${table.short_name},table) ": action name " T4LIT(mismatch,warning) ": " T4LIT(%s,action) ", expected one of ($valid_actions).\n", action_name);
    #[         return false;
    #}     }

    #[     return true;
    #} }
    #[

for table in hlir.tables:
    tmt = table.matchType.name
    #{ void ${table.name}_add_table_entry(p4_ctrl_msg_t* ctrl_m) {
    #[     ENTRY(${table.name}) entry;
    #[     bool success = ${table.name}_setup_entry(&entry, (p4_action_parameter_t**)ctrl_m->action_params, ctrl_m->action_name);
    #[     if (unlikely(!success))    return;
    #[

    table_extra_t = {'exact': '', 'lpm': 'int prefix_length = ', 'ternary': ''}
    extra_names = {'exact': [], 'lpm': ['prefix_length'], 'ternary': []}

    if len(table.key.keyElements) > 0:
        #[     table_key_${table.name}_t key;
        #[     ${table_extra_t[tmt]}${table.name}_setup_key((p4_field_match_${tmt}_t**)ctrl_m->field_matches, &key);
        #[

    extra_params = "".join(f'{p}, ' for p in extra_names[tmt])
    has_fields = "false" if len(action.action_object.parameters.parameters) == 0 else "true"
    keyparam = '(uint8_t*)&key' if len(table.key.keyElements) else 'NULL /* empty key */'
    #[     ${table.matchType.name}_add_promote(TABLE_${table.name}, $keyparam, ${extra_params} (ENTRYBASE*)&entry, false, ${has_fields} || ctrl_is_initialized);

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
#[             debug("   " T4LIT(!!,warning) " Possible table names: $all_keyed_table_names\n");
#[         } else {
#[             debug("   " T4LIT(!!,warning) " Possible table names: $common_keyed_table_names and " T4LIT(%d) " hidden tables\n", $hidden_table_count);
#}         }
#[         possible_tables_already_shown = true;
#}     }

#} #endif

#{ void ctrl_add_table_entry(p4_ctrl_msg_t* ctrl_m) {
for table in hlir.tables:
    #{     if (strcmp("${table.canonical_name}", ctrl_m->table_name) == 0) {
    #[         ${table.name}_add_table_entry(ctrl_m);
    #[         return;
    #}     }
#[     debug(" $$[warning]{}{!!!! Table add entry}: $$[warning]{}{unknown table name} $$[table]{}{%s}\n", ctrl_m->table_name);
#{     #ifdef T4P4S_DEBUG
#[         debug_show_possible_tables();
#}     #endif
#} }


#[ extern char* action_names[];

#{ void ctrl_setdefault(p4_ctrl_msg_t* ctrl_m) {
for table in hlir.tables:
    #{     if (strcmp("${table.canonical_name}", ctrl_m->table_name) == 0) {
    #[         ${table.name}_action_t default_entry;
    #[         make_${table.name}_set_default_table_entry(&default_entry, ctrl_m->action_name, (p4_action_parameter_t**)ctrl_m->action_params);
    #[         table_setdefault_promote(TABLE_${table.name}, (ENTRYBASE*)&default_entry, false);
    #[         return;
    #}     }

#[     debug(" $$[warning]{}{!!!! Table set default}: $$[warning]{}{unknown table name} $$[table]{}{%s}\n", ctrl_m->table_name);
#{     #ifdef T4P4S_DEBUG
#[         debug_show_possible_tables();
#}     #endif
#} }


for inst in hlir.smem_insts.filterfalse('smem.smem_type', ('register', 'direct_counter', 'direct_meter')):
    #[ uint32_t ${get_smem_name(inst, ['ctrl'])}[${inst.amount}];

for packets_or_bytes in ('packets', 'bytes'):
    #{ uint32_t* read_counter_value_by_name_${packets_or_bytes}(char* counter_name, int* size, bool is_bytes) {
    for inst in hlir.smem_insts.filterfalse('smem.smem_type', ('register', 'direct_counter', 'direct_meter')):
        #{ if (strcmp("${inst.smem.canonical_name}", counter_name) == 0) {
        #[     *size = ${inst.amount};
        #[     for (int i=0;i<${inst.amount};++i) ${get_smem_name(inst, ['ctrl'])}[i] = rte_atomic32_read(&global_smem.${get_smem_name(inst.smem)}[i].${packets_or_bytes});
        #[     return ${get_smem_name(inst, ['ctrl'])};
        #} }
        #[
    #[     *size = -1;
    #[     return 0;
    #} }
    #[

#{ uint32_t* read_counter_value_by_name(char* counter_name, int* size, bool is_bytes) {
#[     return is_bytes ? read_counter_value_by_name_bytes(counter_name, size, is_bytes) : read_counter_value_by_name_packets(counter_name, size, is_bytes);
#} }
#[


#[ extern struct socket_state state[NB_SOCKETS];

#[ extern volatile bool ctrl_is_initialized;

#{ void ctrl_initialized() {
#[     debug("   " T4LIT(::,incoming) " Control plane init " T4LIT(done,success) "\n");
#[     ctrl_is_initialized = true;
#} }


#{ void recv_from_controller(p4_ctrl_msg_t* ctrl_m) {
#{     if (ctrl_m->type == P4T_ADD_TABLE_ENTRY) {
#[         ctrl_add_table_entry(ctrl_m);
#[     } else if (ctrl_m->type == P4T_SET_DEFAULT_ACTION) {
#[         ctrl_setdefault(ctrl_m);
#[     } else if (ctrl_m->type == P4T_CTRL_INITIALIZED) {
#[         ctrl_initialized();
#[     } else if (ctrl_m->type == P4T_READ_COUNTER) {
#[         //ctrl_m->xid = *read_counter_value_by_name(ctrl_m->table_name);
#[         //TODO:SEND BACK;
#[     } else {
#[         debug(" " T4LIT(!!!!) " Unknown message (type %d) arrived from the controller\n", ctrl_m->type);
#}     }
#} }


#[ ctrl_plane_backend bg;

#{ void print_table_summary() {
#{     #ifdef T4P4S_DEBUG
#{         for (int i = 0; i < NB_TABLES; i++) {
#[             lookup_table_t t = table_config[i];
#[             if (state[0].tables[t.id][0]->init_entry_count > 0)
#[                 debug("    " T4LIT(:,incoming) " Table " T4LIT(%s,table) " got " T4LIT(%d) " entries from the control plane\n", state[0].tables[t.id][0]->short_name, state[0].tables[t.id][0]->init_entry_count);
#}             }
#}     #endif
#} }
#[

#{ void init_control_plane() {
#[     bg = create_backend(3, 1000, "localhost", T4P4S_CTL_PORT, recv_from_controller);
#[     launch_backend(bg);
#{     #ifdef T4P4S_P4RT
#[         dev_mgr_init_with_t4p4s(dev_mgr_ptr, recv_from_controller, read_counter_value_by_name, 1);
#[         PIGrpcServerRunAddrGnmi("0.0.0.0:50051", 0);
#[         //PIGrpcServerRun();
#[     #else
#[         print_table_summary();
#}     #endif
#} }
#[
