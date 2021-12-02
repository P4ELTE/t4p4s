# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_declaration, format_statement, format_expr, format_type, gen_format_type, get_method_call_env
from utils.extern import get_smem_name
from compiler_log_warnings_errors import addError, addWarning
from compiler_common import types, generate_var_name, get_hdrfld_name, unique_everseen


#[ #include "gen_include.h"
#[ #include "dataplane_impl.h"

table_infos = [(table, table.short_name + ("/keyless" if table.key_bit_size == 0 else "") + ("/hidden" if table.is_hidden else "")) for table in hlir.tables]

for table, table_info in table_infos:
    if len(table.direct_meters + table.direct_counters) == 0:
        continue

    #{ void ${table.name}_apply_smems(STDPARAMS) {
    #[     // applying direct counters and meters
    for table2, smem in hlir.smem.directs:
        if table != table2:
            continue

        # note: the last arg is translated into macro invocations like this: STR_SMEM3(...)
        #[     apply_${smem.smem_type}(&(global_smem.${get_smem_name(smem)}[0]), 1, pd->parsed_size, "${table.name}", "${smem.smem_type}", STR_${get_smem_name(smem)});
    #} }
    #[
