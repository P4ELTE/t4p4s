# SPDX-License-Identifier: Apache-2.0
# Copyright 2018 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_expr, format_statement, format_declaration

#[ #include "stateful_memory.h"

#[ global_state_t global_smem;

#{ void gen_init_smems() {
#[ #ifdef T4P4S_DEBUG
for table, smem in hlir.all_meters + hlir.all_counters:
    for target in smem.smem_for:
        if not smem.smem_for[target]:
            continue
        table_postfix = f"_{table.name}" if smem.is_direct else ""

        for c in smem.components:
            cname = c['name']
            if not smem.is_direct:
                #{ for (int idx = 0; idx < ${smem.amount}; ++idx) {
                #[     strcpy(global_smem.${cname}[idx].name, "${smem.name}/${c['for']}");
                #} }

                #[ global_smem.${cname}_amount = ${smem.amount};
            else:
                #[ strcpy(global_smem.${cname}_${table.name}.name, "${smem.name}/${c['for']}");

    #[

for smem in hlir.registers:
    for c in smem.components:
        cname = c['name']
        #{ for (int idx = 0; idx < ${smem.amount}; ++idx) {
        #[     strcpy(global_smem.${smem.smem_type}_${cname}[idx].name, "${smem.name}");
        #} }

        #[ global_smem.${smem.smem_type}_${smem.name}_amount = ${smem.amount};

    #[

#[ #endif
#} }
