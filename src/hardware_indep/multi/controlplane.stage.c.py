# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

compiler_common.current_compilation['is_multicompiled'] = True

part_count = compiler_common.current_compilation['multi']
multi_idx = compiler_common.current_compilation['multi_idx']

all_tables = sorted(hlir.tables, key=lambda table: len(table.actions))
tables = list(table for idx, table in enumerate(all_tables) if idx % part_count == multi_idx)

if tables == []:
    compiler_common.current_compilation['skip_output'] = True
else:
    from compiler_common import unique_everseen, generate_var_name, get_hdr_name, get_hdrfld_name
    from utils.codegen import format_expr, format_type, gen_format_slice

    import os

    #[ #include <unistd.h>

    #[ #include "dpdk_lib.h"
    #[ #include "dpdk_primitives.h" // TODO remove
    #[ #include "actions.h"
    #[ #include "tables.h"

    for table in tables:
        #{ void ${table.name}_set_default_table_action(${table.name}_action_t* action, const char* action_name, p4_action_parameter_t** action_params) {
        for action in table.actions:
            ao = action.action_object

            #{     if (strcmp("${ao.canonical_name}", action_name) == 0) {
            #[         action->action_id = action_${ao.name};

            for j, p in enumerate(ao.parameters.parameters):
                #[         uint8_t* param_${p.name} = (uint8_t*)action_params[$j]->bitmap;
                #[         memcpy(&action->${ao.name}_params.${p.name}, param_${p.name}, ${(p.urtype.size+7)//8});

            if not table.is_hidden:
                #[         debug(" " T4LIT(ctl>,incoming) " Set " T4LIT(default action,action) " for $$[table]{table.short_name}: $$[action]{ao.short_name}\n");

            #[         return;

            #}     }

        valid_actions = ", ".join([f'" T4LIT({a.action_object.canonical_name},action) "' for a in table.actions])
        #[
        #[     debug("   $$[warning]{}{!! Table setdefault} on table " T4LIT(%s,table) ": action name $$[warning]{}{mismatch} " T4LIT(%s,action) ", expected one of ($valid_actions)\n", "${table.short_name}", action_name);
        #[         action->action_id = INVALID_ACTION;
        #} }
        #[
