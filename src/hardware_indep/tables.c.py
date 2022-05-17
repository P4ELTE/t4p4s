# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_expr
import utils.codegen
from compiler_log_warnings_errors import addError, addWarning
from compiler_common import generate_var_name, prepend_statement, SugarStyle, to_c_bool, make_const

#[ #include "dataplane.h"
#[ #include "actions.h"
#[ #include "tables.h"
#[ #include "stateful_memory.h"
#[ #include "dpdk_lib.h"
#[ #include "util_debug.h"
#[



all_table_short_names_sorted = ' ", " '.join(sorted(f'T4LIT({table.short_name},table)' for table in hlir.tables))
table_short_names_sorted = ' ", " '.join(sorted(f'T4LIT({table.short_name},table)' for table in hlir.tables if not table.is_hidden))
#{ #ifdef T4P4S_DEBUG
#[     const char* all_table_short_names_sorted = "" ${all_table_short_names_sorted};
#[     const int   all_table_short_names_count  = ${len(hlir.tables)};
#[     const char* table_short_names_sorted     = "" ${table_short_names_sorted};
#[     const int   table_short_names_count      = ${len(hlir.tables.filterfalse('is_hidden'))};
#} #endif
#[

with SugarStyle("no_comment"):
    #{ #define TABLE_CONFIG_ENTRY_DEF(tname,cname,sname,mt,hidden,keysize,size) (lookup_table_t) { \
    #[  .name           = #tname, \
    #[  .canonical_name = #cname, \
    #[  .short_name     = #sname, \
    #[  .id = TABLE_ ## tname, \
    #[  .type = LOOKUP_ ## mt, \
    #[  .default_val = NULL, \
    #[  .is_hidden = hidden, \
    #{  .entry = { \
    #[      .entry_count = 0, \
    #[      .key_size = keysize, \
    #[      .action_size = sizeof(tname ## _action_t), \
    #[      .state_size = 0, \
    #}      }, \
    #[  .min_size = 0, \
    #[  .max_size = size == NO_TABLE_SIZE ? MAX_TABLE_SIZE : size, \
    #} }
    #[


#[ lookup_table_t table_config[NB_TABLES] = {
for table in hlir.tables:
    tmt = table.matchType.name
    ks  = table.key_length_bytes
    size = table.size.expression.value if 'size' in table else 'NO_TABLE_SIZE'
    #[     TABLE_CONFIG_ENTRY_DEF(${table.name},${table.canonical_name},${table.short_name},$tmt,${to_c_bool(table.is_hidden)},$ks,$size),
#[ };


for table in hlir.tables:
    #{ void setdefault_${table.name}(actions_e action_id, bool show_info) {
    #[     ENTRY(${table.name}) default_entry = { .id = action_id };
    #[     table_setdefault_promote(TABLE_${table.name}, (ENTRYBASE*)&default_entry, show_info);
    #} }


#[ #define SOCKET0 0

#[ extern struct socket_state state[NB_SOCKETS];
#[

nops = list(sorted((t for t in hlir.tables if not t.is_hidden for default in [t.default_action.expression.method.action_ref] if default.canonical_name == '.nop'), key=lambda t: t.short_name))
nopinfo = "" if len(nops) == 0 else f' ({len(nops)} " T4LIT(nop,action) " defaults: ' + ", ".join(f'" T4LIT({t.short_name},table) "' for t in nops) + ')'
tableinfos = [(t.name, t not in nops and not t.is_hidden, t.default_action.expression.method.action_ref) for t in sorted(hlir.tables, key=lambda table: table.short_name)]

#{ void init_table_default_actions() {
if len(nops) > 0:
    noptxt = ", ".join(f'" T4LIT({t.short_name},table) "' for t in nops)
    #[     debug(" :::: Init tables: " T4LIT(${len(nops)}) " " T4LIT(nop,action) " default actions: $noptxt\n");
if len(definfos := list((tname, default_action.name) for tname, show_info, default_action in tableinfos if show_info if default_action.name != '.NoAction')) > 0:
    deftxt = ", ".join(f'" T4LIT({tname},table) "[" T4LIT({defname},action) "]' for tname, defname in definfos)

if len(tableinfos) > 0:
    #[     struct socket_state socket0 = state[SOCKET0];

for name, show_info, default_action in tableinfos:
    #[     int current_replica_${name} = socket0.active_replica[TABLE_${name}];
    #{     if (likely(socket0.tables[TABLE_${name}][current_replica_${name}]->default_val == NULL)) {
    #[         setdefault_${name}(action_${default_action.name}, false);
    #}     }
#} }
#[


# TODO move to a utility module
def align_to_byte(num):
    return (num + 7) // 8


def gen_make_const_entry(entry, params, args, keys, key_sizes, varinfos, action_id):
    # note: _left is for lpm and ternary that may have a mask
    key_total_size = sum(entry.keys.components.map('_left.urtype.size').map(align_to_byte))
    # key_total_size = (sum((key._left.urtype.size for key in entry.keys.components))+7) // 8

    #[     uint8_t ${key_var}[${key_total_size}];

    offsets = ["+".join(["0"] + [f'{ksize}' for ksize in key_sizes[0:idx]]) for idx, ksize in enumerate(key_sizes)]

    for key, ksize, (const_var, hex_content) in zip(keys, key_sizes, varinfos):
        #[     uint8_t ${const_var}[] = {$hex_content};

    for key, ksize, offset, (const_var, hex_content) in zip(keys, key_sizes, offsets, varinfos):
        #[     memcpy(${key_var} + ((${offset} +7)/8), &${const_var}, ${(ksize+7)//8});

    #{     ENTRY(${table.name}) ${entry_var} = {
    #[         .id = action_${action_id},
    #{         .params = {
    #{             .${action_id}_params = {
    for param, value_expr in zip(params, args):
        _, hex_content = make_const(value_expr.expression)
        if param.urtype.size <= 32:
            #[                 .${param.name} = ${value_expr.expression.value},
        else:
            #[                 .${param.name} = { ${hex_content} }, // ${value_expr.expression.value}
    #}             },
    #}         },
    #}     };


def gen_add_const_entry(table, key_var, entry_var, keys, key_sizes, varinfos, mt):
    if mt == 'exact':
        #[     ${mt}_add_promote(TABLE_${table.name}, ${key_var}, (ENTRYBASE*)&${entry_var}, true, false);
    elif mt == 'lpm':
        # TODO: if there are exact fields as well as an lpm field, make sure that the exact fields are in front
        lpm_depth = sum((f'{key.right.value:b}'.count('1') if key.node_type == 'Mask' else ksize for key, ksize, (const_var, hex_content) in zip(keys, key_sizes, varinfos)))
        #[     ${mt}_add_promote(TABLE_${table.name}, ${key_var}, ${lpm_depth}, (ENTRYBASE*)&${entry_var}, true, false);
    elif mt == 'ternary':
        ternary_expr = keys[0].right
        #[     ${mt}_add_promote(TABLE_${table.name}, ${key_var}, ${format_expr(ternary_expr)}, (ENTRYBASE*)&${entry_var}, true, false);


def gen_print_const_entry(table, entry, params, args, mt):
    def make_value(value):
        if value.type.node_type in ('Type_Enum', 'Type_Error'):
            values = value.type.members
            elem = values.get(value.member)
            idx = {val: idx for idx, val in enumerate(values)}[elem]
            return f'{value.member}={idx}'
        elif value.node_type == 'BoolLiteral':
            value_const = 1 if value.value else 0
            value_base = 2
        else:
            value_const = value.value
            value_base = value.base

        is_hex = value_base == 16
        split_places = 4 if is_hex else 3

        val = f'{value_const:x}' if is_hex else f'{value_const}'
        val = '_'.join(val[::-1][i:i+split_places] for i in range(0, len(val), split_places))[::-1]
        return f'0x{val},bytes' if is_hex else f'{val}'

    def make_key(table, key, value):
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

        long_name = key.expression.path.name
        short_name = table.control.controlLocals.get(long_name, 'Declaration_Variable').short_name
        return f'" T4LIT({short_name},field) "={value_str}{mask_str}'

    def make_param(param, value_expr):
        return f'" T4LIT({param.name},field) "=" T4LIT({make_value(value_expr.expression)}) "'

    key_str = ", ".join((make_key(table, key, value) for key, value in zip(table.key.keyElements, entry.keys.components)))
    params_str = ", ".join((make_param(param, value_expr) for param, value_expr in zip(params, args)))
    if params_str != "":
        params_str = f'({params_str})'

    #{     #ifndef TEST_CONST_ENTRIES_hide
    #{     #ifndef TEST_CONST_ENTRIES_simple
    #[         debug("   :: Table " T4LIT(${table.short_name},table) "/" T4LIT($mt) ": const entry (${key_str}) -> " T4LIT(${entry.action.method.action_ref.short_name},action) "${params_str}\n");
    #}     #endif
    #}     #endif
#[

#[ char summary[1024];

#{ void print_const_entry_summary() {
#{     #ifdef TEST_CONST_ENTRIES_simple
#[         char* summary_ptr = summary;
#[         int count = 0;
#[         bool is_first = true;
for table in hlir.tables:
    if 'entries' not in table:
        continue

    #[         int ${table.name}_size = table_size(TABLE_${table.name});
    #{         if (${table.name}_size > 0) {
    #[             count += ${table.name}_size;
    #[             summary_ptr += sprintf(summary_ptr, "%s" T4LIT(%d) " on " T4LIT(${table.short_name},table), is_first ? "" : ", ", ${table.name}_size);
    #[             is_first = false;
    #}         }
#{         if (count > 0) {
#[             debug(" :::: Const entries on tables: %s\n", summary);
#}         }

#}     #endif
#} }
#[

for table in hlir.tables:
    if 'entries' not in table:
        continue

    if 'size' in table and (table_size := table.size.expression.value) < (const_entry_count := len(table.entries.entries)):
        addError("Generating const entries", f"Table {table.name} has {const_entry_count} const entries, but its size is only {table_size}")

    #{ void init_table_const_entries_${table.name}() {
    for entry in table.entries.entries:
        if any((component.urtype.node_type == 'Type_Dontcare' for component in entry.keys.components)):
            addWarning("adding const entry", f"Underscore entry for const entry for table {table.name} not supported yet")
            continue

        utils.codegen.pre_statement_buffer = ""

        action_id = entry.action.method.path.name

        key_var = generate_var_name("key", f"{table.name}__{action_id}")
        entry_var = generate_var_name("entry", f"{table.name}__{action_id}")

        params = entry.action.method.type.parameters.parameters
        args = entry.action.arguments

        mt = table.matchType.name

        keys = entry.keys.components
        key_sizes = [key._left.urtype.size for key in keys]

        def make_var(keyelem, key, ksize):
            is_meta = 'header' in keyelem and keyelem.header.name == 'all_metadatas'
            name, hex_content = make_const(key._left, not is_meta and ksize <= 32)
            const_var = generate_var_name(f"const{ksize}", name)
            return const_var, hex_content

        varinfos = [make_var(keyelem, key, ksize) for keyelem, key, ksize in zip(table.key.keyElements, keys, key_sizes)]

        #[ ${utils.codegen.pre_statement_buffer}
        #[ ${gen_make_const_entry(entry, params, args, keys, key_sizes, varinfos, action_id)}
        #[ ${gen_add_const_entry(table, key_var, entry_var, keys, key_sizes, varinfos, mt)}
        #[ ${gen_print_const_entry(table, entry, params, args, mt)}

        utils.codegen.pre_statement_buffer = ""
    #} }
    #[


#{ void init_table_const_entries() {
for table in hlir.tables:
    if 'entries' not in table:
        #[     // no const entries in table ${table.name}
        continue
    #[     init_table_const_entries_${table.name}();
#[     print_const_entry_summary();
#} }

#[ // ============================================================================
#[ // Getters

#[ extern char* action_names[];
#[ extern char* action_canonical_names[];
#[ extern char* action_short_names[];
