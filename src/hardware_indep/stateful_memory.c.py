# Copyright 2018 Eotvos Lorand University, Budapest, Hungary
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

from utils.codegen import format_expr, format_statement, statement_buffer_value, format_declaration

#[ #include "stateful_memory.h"

#[ global_state_t global_smem;

#{ void gen_init_smems() {
#[ #ifdef T4P4S_DEBUG
for table, smem in hlir16.all_meters + hlir16.all_counters:
    for target in smem.smem_for:
        if not smem.smem_for[target]:
            continue
        table_postfix = "_{}".format(table.name) if smem.smem_type in ["direct_counter", "direct_meter"] else ""

        for c in smem.components:
            cname = c['name']
            if smem.smem_type not in ["direct_counter", "direct_meter"]:
                #{ for (int idx = 0; idx < ${smem.amount}; ++idx) {
                #[     strcpy(global_smem.${cname}[idx].name, "${smem.name}/${c['for']}");
                #} }

                #[ global_smem.${cname}_amount = ${smem.amount};
            else:
                #[ strcpy(global_smem.${cname}_${table.name}.name, "${smem.name}/${c['for']}");

    #[

for smem in hlir16.registers:
    for c in smem.components:
        #{ for (int idx = 0; idx < ${smem.amount}; ++idx) {
        #[     strcpy(global_smem.${smem.name}[idx].name, "${smem.name}");
        #} }

        #[ global_smem.${smem.name}_amount = ${smem.amount};

    #[

#[ #endif
#} }
