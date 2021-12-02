# SPDX-License-Identifier: Apache-2.0
# Copyright 2018 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_expr, format_statement, format_declaration
from utils.extern import get_smem_name
from compiler_common import unique_everseen

#[ #include "stateful_memory.h"

#[ global_state_t global_smem;

for stype in unique_everseen(smem.smem_type for table, smem in hlir.smem.all_meters + hlir.smem.all_counters):
    #{ void init_${stype}_smem(SMEMTYPE($stype) smem[], int amount, const char*const name, const char*const purpose) {
    #{     for (int idx = 0; idx < amount; ++idx) {
    #[         sprintf(smem[idx].name, "%s/%s", name, purpose);
    #}     }
    #} }
    #[


#{ void gen_init_smems() {
#[ #ifdef T4P4S_DEBUG
for _, smem in hlir.smem.all_meters + hlir.smem.all_counters:
    if smem.is_direct:
        #[     strcpy(global_smem.${get_smem_name(smem)}[0].name, "${smem.name}/${smem.packets_or_bytes}");
    else:
        #[     init_${smem.smem_type}_smem(global_smem.${get_smem_name(smem)}, ${smem.amount}, "${smem.name}", "${smem.packets_or_bytes}");

#[

for reg in hlir.smem.registers:
    #{     for (int idx = 0; idx < ${reg.amount}; ++idx) {
    #[         strcpy(global_smem.${get_smem_name(reg)}[idx].name, "${reg.name}");
    #}     }

    #[

#[ #endif
#} }
