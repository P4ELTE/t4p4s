# SPDX-License-Identifier: Apache-2.0
# Copyright 2021 Eotvos Lorand University, Budapest, Hungary

compiler_common.current_compilation['is_multicompiled'] = True

part_count = compiler_common.current_compilation['multi']
multi_idx = compiler_common.current_compilation['multi_idx']

table_names = (table.short_name + ("/keyless" if table.key_length_bits == 0 else "") + ("/hidden" if table.is_hidden else "") for table in hlir.tables)
all_table_infos = sorted(zip(hlir.tables, table_names), key=lambda k: len(k[0].actions))
table_infos = list(ti for idx, ti in enumerate(all_table_infos) if idx % part_count == multi_idx)

all_ctl_stages = ((ctl, idx, comp) for ctl in hlir.controls for idx, comp in enumerate(ctl.body.components))
ctl_stages = list(cic for idx, cic in enumerate(all_ctl_stages) if (idx + len(all_table_infos)) % part_count == multi_idx)

if table_infos == [] and ctl_stages == []:
    compiler_common.current_compilation['skip_output'] = True
else:
    from utils.codegen import format_declaration, format_statement, format_expr, format_type, gen_format_type, get_method_call_env
    from compiler_log_warnings_errors import addError, addWarning
    from compiler_common import types, generate_var_name, get_hdrfld_name, unique_everseen

    #[ #include "dataplane_impl.h"
    #[ #include "gen_model.h"

    # TODO make this an import from hardware_indep
    #[ #include "dpdk_smem.h"
    #[

    for table, table_info in table_infos:
        #{     apply_result_t ${table.name}_apply(STDPARAMS) {
        if 'key' not in table or table.key_length_bits == 0:
            #[         table_entry_${table.name}_t* entry = ${table.name}_get_default_entry(STDPARAMS_IN);
            #[         bool hit = false;
            #[         ${table.name}_apply_show_hit(entry->action.action_id, STDPARAMS_IN);
        else:
            #[         uint8_t* key[${table.key_length_bytes}];
            #[         table_${table.name}_key(pd, (uint8_t*)key);

            #[         table_entry_${table.name}_t* entry = (table_entry_${table.name}_t*)${table.matchType.name}_lookup(tables[TABLE_${table.name}], (uint8_t*)key);
            #[         bool hit = entry != NULL && entry->is_entry_valid != INVALID_TABLE_ENTRY;
            #{         if (unlikely(!hit)) {
            #[             entry = ${table.name}_get_default_entry(STDPARAMS_IN);
            #}         }

            #{         #ifdef T4P4S_DEBUG
            #[             if (likely(entry != 0))    ${table.name}_apply_show_hit_with_key(key, hit, entry, STDPARAMS_IN);
            #}         #endif

            #{         #ifdef T4P4S_STATS
            #[             t4p4s_stats_global.table_hit__${table.name} = hit || t4p4s_stats_global.table_hit__${table.name};
            #[             t4p4s_stats_global.table_miss__${table.name} = !hit || t4p4s_stats_global.table_miss__${table.name};
            #[             t4p4s_stats_per_packet.table_hit__${table.name} = hit || t4p4s_stats_per_packet.table_hit__${table.name};
            #[             t4p4s_stats_per_packet.table_miss__${table.name} = !hit || t4p4s_stats_per_packet.table_miss__${table.name};
            #}         #endif

        if len(table.direct_meters + table.direct_counters) > 0:
            #[         if (likely(hit))    ${table.name}_apply_smems(STDPARAMS_IN);

        #[         if (unlikely(entry == 0))    return (apply_result_t) { hit, -1 /* invalid action */ };

        #[         ${table.name}_stats(entry->action.action_id, STDPARAMS_IN);

        # ACTIONS
        if len(table.actions) == 1:
            ao = table.actions[0].action_object
            if len(ao.body.components) != 0:
                #[         action_code_${ao.name}(entry->action.${ao.name}_params, SHORT_STDPARAMS_IN);
            #[         return (apply_result_t) { hit, action_${ao.name} };
        else:
            #{         switch (entry->action.action_id) {
            for action in table.actions:
                ao = action.action_object
                #{       case action_${ao.name}:
                if len(ao.body.components) != 0:
                    #[               action_code_${ao.name}(entry->action.${ao.name}_params, SHORT_STDPARAMS_IN);
                #}               return (apply_result_t) { hit, action_${ao.name} };
            #[         }
            #}         return (apply_result_t) {}; // unreachable

        #}     }
        #[

    ################################################################################

    for ctl, idx, comp in ctl_stages:
        #{ void control_stage_${ctl.name}_${idx}(control_locals_${ctl.name}_t* local_vars, STDPARAMS)  {
        #[     uint32_t value32, res32;
        #[     (void)value32, (void)res32;
        compiler_common.enclosing_control = ctl
        #= format_statement(comp, ctl)
        compiler_common.enclosing_control = None
        #} }
        #[
