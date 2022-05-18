# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from compiler_common import unique_everseen, generate_var_name, get_hdr_name, get_hdrfld_name
from utils.codegen import format_expr, format_type, gen_format_slice

import os

#[ #include <unistd.h>

#[ #include "dpdk_lib.h"
#[ #include "dpdk_primitives.h" // TODO remove
#[ #include "actions.h"
#[ #include "tables.h"

part_count = compiler_common.current_compilation['multi']
all_tables = sorted(hlir.tables, key=lambda table: len(table.actions))
for idx, table in enumerate(all_tables):
    multi_idx = idx % part_count
    #{ #if T4P4S_MULTI_IDX == ${multi_idx}

    #{ void make_${table.name}_set_default_table_entry(ENTRY(${table.name})* entry, const char* action_name, p4_action_parameter_t** action_params) {
    for action in table.actions:
        ao = action.action_object

        #{     if (strcmp("${ao.canonical_name}", action_name) == 0) {
        #[         entry->id = action_${ao.name};

        for idx, par in enumerate(ao.parameters.parameters):
            #[         uint8_t* param_${par.name} = (uint8_t*)action_params[$idx]->bitmap;

        for par in ao.parameters.parameters:
            #[         memcpy(&entry->params.${ao.name}_params.${par.name}, param_${par.name}, ${(par.urtype.size+7) // 8});

        if not table.is_hidden:
            #[         debug(" " T4LIT(ctl>,incoming) " Set " T4LIT(default action,action) " for " T4LIT(${table.short_name},table) ": " T4LIT(${ao.short_name},action) "\n");

        #[         return;

        #}     }

    valid_actions = ", ".join([f'" T4LIT({a.action_object.canonical_name},action) "' for a in table.actions])
    #[
    #[     debug("   " T4LIT(!!,warning) " Table setdefault on table " T4LIT(${table.short_name},table) ": action name " T4LIT(mismatch,warning) " " T4LIT(%s,action) ", expected one of ($valid_actions)\n", action_name);
    #[     entry->id = INVALID_ACTION;
    #} }
    #} #endif // T4P4S_MULTI_IDX == ${multi_idx}
    #[
