# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_expr, make_const
import utils.codegen
from compiler_log_warnings_errors import addError, addWarning
from compiler_common import generate_var_name, prepend_statement

#[ #include "dataplane.h"
#[ #include "actions.h"
#[ #include "tables.h"
#[ #include "stateful_memory.h"
#[ #include "dpdk_lib.h"
#[ #include "util_debug.h"
#[

#[ lookup_table_t table_config[NB_TABLES] = {
for table in hlir.tables:
    tmt = table.matchType.name
    ks  = table.key_length_bytes
    #[ {
    #[  .name           = "${table.name}",
    #[  .canonical_name = "${table.canonical_name}",

    #[  .id = TABLE_${table.name},
    #[  .type = LOOKUP_$tmt,

    #[  .default_val = NULL,

    #[  .is_hidden = ${"true" if table.is_hidden else "false"},

    #[  .entry = {
    #[      .entry_count = 0,

    #[      .key_size = $ks,

    #[      .entry_size = sizeof(${table.name}_action_t) + sizeof(entry_validity_t),
    #[      .action_size   = sizeof(${table.name}_action_t),
    #[      .validity_size = sizeof(entry_validity_t),
    #[  },

    #[  .min_size = 0,
    #[  .max_size = 250000,

    #{  #ifdef T4P4S_DEBUG
    #[      .short_name= "${table.short_name}",
    #}  #endif
    #[ },
#[ };


for table in hlir.tables:
    #{ void setdefault_${table.name}(actions_t action_id) {
    #{     table_entry_${table.name}_t default_action = {
    #[          .action = { action_id },
    #[          .is_entry_valid = VALID_TABLE_ENTRY,
    #}     };
    #[     table_setdefault_promote(TABLE_${table.name}, (actions_t*)&default_action);
    #} }


#[ #define SOCKET0 0

#[ extern struct socket_state state[NB_SOCKETS];
#[ extern void table_setdefault_promote(table_name_t tableid, actions_t* action_id);

#{ void init_table_default_actions() {
#[     debug(" :::: Init table default actions\n");

for table in hlir.tables:
    #[     int current_replica_${table.name} = state[SOCKET0].active_replica[TABLE_${table.name}];
    #{     if (likely(state[SOCKET0].tables[TABLE_${table.name}][current_replica_${table.name}]->default_val == NULL)) {
    #[         setdefault_${table.name}(action_${table.default_action.expression.method.action_ref.name});
    #}     }
#} }
#[

for table in hlir.tables:
    if 'entries' not in table:
        continue

    #{ void init_table_const_entries_${table.name}() {
    for entry in table.entries.entries:
        if any((component.urtype.node_type == 'Type_Dontcare' for component in entry.keys.components)):
            addWarning("adding const entry", f"Underscore entry for const entry for table {table.name} not supported yet")
            continue

        utils.codegen.pre_statement_buffer = ""

        action_id = entry.action.method.path.name

        key_total_size = (sum((key._left.urtype.size for key in entry.keys.components))+7) // 8

        # note: _left is for lpm and ternary that may have a mask
        key_var = generate_var_name("key", f"{table.name}__{action_id}")
        action_var = generate_var_name("action", f"{table.name}__{action_id}")

        params = entry.action.method.type.parameters.parameters
        args = entry.action.arguments

        #[     ${utils.codegen.pre_statement_buffer}

        #[     uint8_t ${key_var}[${key_total_size}];

        def make_var(key, ksize):
            name, hex_content = make_const(key._left)
            const_var = generate_var_name(f"const{ksize}", name)
            return const_var, hex_content

        keys = entry.keys.components
        key_sizes = [key._left.urtype.size for key in keys]
        offsets = ["+".join(["0"] + [f'{ksize}' for ksize in key_sizes[0:idx]]) for idx, ksize in enumerate(key_sizes)]
        varinfos = [make_var(key, ksize) for key, ksize in zip(keys, key_sizes)]

        for key, ksize, (const_var, hex_content) in zip(keys, key_sizes, varinfos):
            #[     uint8_t ${const_var}[] = {$hex_content};

        for key, ksize, offset, (const_var, hex_content) in zip(keys, key_sizes, offsets, varinfos):
            #[     memcpy(${key_var} + ((${offset} +7)/8), &${const_var}, ${(ksize+7)//8});

        #{     ${table.name}_action_t ${action_var} = {
        #[         .action_id = action_${action_id},
        #{         .${action_id}_params = {
        for param, value_expr in zip(params, args):
            _, hex_content = make_const(value_expr.expression)
            if param.urtype.size <= 32:
                #[             .${param.name} = ${value_expr.expression.value},
            else:
                #[             .${param.name} = { ${hex_content} }, // ${value_expr.expression.value}
        #}         },
        #}     };

        mt = table.matchType.name
        if mt == 'exact':
            #[     ${mt}_add_promote(TABLE_${table.name}, ${key_var}, (uint8_t*)&${action_var}, true, false);
        elif mt == 'lpm':
            # TODO: if there are exact fields as well as an lpm field, make sure that the exact fields are in front
            lpm_depth = sum((f'{key.right.value:b}'.count('1') if key.node_type == 'Mask' else ksize for key, ksize, (const_var, hex_content) in zip(keys, key_sizes, varinfos)))
            #[     ${mt}_add_promote(TABLE_${table.name}, ${key_var}, ${lpm_depth}, (uint8_t*)&${action_var}, true, false);
        elif mt == 'ternary':
            ternary_expr = keys[0].right
            #[     ${mt}_add_promote(TABLE_${table.name}, ${key_var}, ${format_expr(ternary_expr)}, (uint8_t*)&${action_var}, true, false);

        def make_value(value):
            is_hex = value.base == 16
            split_places = 4 if is_hex else 3

            prefix = '0x' if is_hex else ''
            val = f'{value.value:x}' if is_hex else f'{value.value}'
            val = '_'.join(val[::-1][i:i+split_places] for i in range(0, len(val), split_places))[::-1]
            return f'{prefix}{val}'

        def make_key(key, value):
            value_str = f'" T4LIT({make_value(value._left)}) "'
            mask_str = ''
            if value.node_type == 'Mask':
                if mt == 'lpm':
                    depth = f'{value.right.value:b}'.count('1')
                    mask_str = f'/" T4LIT({depth}b) "'
                if mt == 'ternary':
                    mask_str = ' &&& " T4LIT({make_value(value.right)}) "'

            if 'header_name' in key:
                return f'" T4LIT({key.header_name},header) "." T4LIT({key.field_name},field) "={value_str}{mask_str}'
            return f'" T4LIT({key.expression.path.name}) "={value_str}{mask_str}'

        def make_param(param, value_expr):
            return f'" T4LIT({param.name},field) "=" T4LIT({make_value(value_expr.expression)}) "'

        key_str = ", ".join((make_key(key, value) for key, value in zip(table.key.keyElements, entry.keys.components)))
        params_str = ", ".join((make_param(param, value_expr) for param, value_expr in zip(params, args)))
        if params_str != "":
            params_str = f'({params_str})'

        #[     debug("   :: Table $$[table]{table.name}/$${}{%s}: const entry (${key_str}) -> $$[action]{action_id}${params_str}\n", "$mt");

        utils.codegen.pre_statement_buffer = ""
    #} }
    #[


#{ void init_table_const_entries() {
for table in hlir.tables:
    if 'entries' not in table:
        #[     // no const entries in table ${table.name}
        continue
    #[     init_table_const_entries_${table.name}();
#} }

#[ // ============================================================================
#[ // Getters

#[ extern char* action_names[];
#[ extern char* action_canonical_names[];

#[ int get_entry_action_id(const void* entry) {
#[     return *((int*)entry);
#[ }

#[ char* get_entry_action_name(const void* entry) {
#[     return action_canonical_names[get_entry_action_id(entry)];
#[ }

#[ bool* entry_validity_ptr(uint8_t* entry, lookup_table_t* t) {
#[     return (bool*)(entry + t->entry.action_size + t->entry.state_size);
#[ }
