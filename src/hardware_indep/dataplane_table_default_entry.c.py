# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_declaration, format_statement, format_expr, format_type, gen_format_type, get_method_call_env
from compiler_log_warnings_errors import addError, addWarning
from compiler_common import types, generate_var_name, get_hdrfld_name, unique_everseen

#[ #include "dataplane_impl.h"

table_infos = [(table, table.short_name + ("/keyless" if table.key_length_bits == 0 else "") + ("/hidden" if table.is_hidden else "")) for table in hlir.tables]

for table, table_info in table_infos:
    # note: default_val is set properly only on lcore 0 on each socket
    #{ table_entry_${table.name}_t* ${table.name}_get_default_entry(STDPARAMS) {
    #[     return (table_entry_${table.name}_t*)tables[TABLE_${table.name}][0].default_val;
    #} }
    #[
