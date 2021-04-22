# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_declaration, format_statement, format_expr, format_type, gen_format_type, get_method_call_env
from compiler_log_warnings_errors import addError, addWarning
from compiler_common import types, generate_var_name, get_hdrfld_name, unique_everseen

#[ #include "dataplane_impl.h"

table_infos = [(table, table.short_name + ("/keyless" if table.key_length_bits == 0 else "") + ("/hidden" if table.is_hidden else "")) for table in hlir.tables]

for table, table_info in table_infos:
    #{ void ${table.name}_stats(int action_id, STDPARAMS) {
    #{     #ifdef T4P4S_STATS
    #[         t4p4s_stats_global.table_apply__${table.name} = true;
    #[         t4p4s_stats_per_packet.table_apply__${table.name} = true;

    for stat_action in table.actions:
        stat_action_name = stat_action.expression.method.path.name
        #{         if (action_${stat_action_name} == action_id) {
        #[             t4p4s_stats_global.table_action_used__${table.name}_${stat_action_name} = true;
        #[             t4p4s_stats_per_packet.table_action_used__${table.name}_${stat_action_name} = true;
        #}         }
    #}     #endif
    #} }
    #[
