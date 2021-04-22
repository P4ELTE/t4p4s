# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_declaration, format_statement, format_expr, format_type, gen_format_type, get_method_call_env
from compiler_log_warnings_errors import addError, addWarning
from compiler_common import types, generate_var_name, get_hdrfld_name, unique_everseen

#[ #include "dataplane_impl.h"

table_infos = [(table, table.short_name + ("/keyless" if table.key_length_bits == 0 else "") + ("/hidden" if table.is_hidden else "")) for table in hlir.tables]

for table, table_info in table_infos:
    if len(table.direct_meters + table.direct_counters) == 0:
        continue

    #{ void ${table.name}_apply_smems(STDPARAMS) {
    #[     // applying direct counters and meters
    for smem in table.direct_meters + table.direct_counters:
        for comp in smem.components:
            value = "pd->parsed_length" if comp['for'] == 'bytes' else "1"
            type  = comp['type']
            name  = comp['name']
            #[     apply_${smem.smem_type}(&(global_smem.${name}_${table.name}), $value, "${table.name}", "${smem.smem_type}", "$name");
    #} }
    #[
