# SPDX-License-Identifier: Apache-2.0
# Copyright 2021 Eotvos Lorand University, Budapest, Hungary

compiler_common.current_compilation['is_multicompiled'] = True

part_count = compiler_common.current_compilation['multi']
multi_idx = compiler_common.current_compilation['multi_idx']

table_names = (table.short_name + ("/keyless" if table.key_bit_size == 0 else "") + ("/hidden" if table.is_hidden else "") for table in hlir.tables)
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
        tname = table.name
        #{     apply_result_t ${tname}_apply(STDPARAMS) {
        if table.key_bit_size == 0:
            #[         ENTRY(${tname})* entry = ${tname}_get_default_entry(STDPARAMS_IN);
            #[         bool hit = true; // empty key in table ${table.name}
            #[         ${tname}_apply_show_hit(entry->id, STDPARAMS_IN);
        else:
            #{         #ifdef T4P4S_DEBUG
            #[             char key_txt[4096];
            #[             int key_txt_idx = 0;
            #}         #endif

            #[         uint8_t key[${table.key_length_bytes}];
            #[         table_${tname}_key(pd, key  KEYTXTPARAMS_IN);

            #[         ENTRY($tname)* entry = (ENTRY($tname)*)${table.matchType.name}_lookup(tables[TABLE_$tname], key);

            noaction_names = table.actions.map('expression.method.path').filter(lambda p: p.name.startswith('NoAction')).map('name')
            if len(noaction_names) == 0:
                #[         bool is_miss = false; // lookup on table ${table.name} cannot miss
            elif len(noaction_names) == 1:
                #[         bool is_miss = entry->id == action_${noaction_names[0]};
            else:
                addError('Finding NoAction', f'Too many NoActions ({len(noaction_names)}) found')

            #[         bool hit = !is_miss;
            #{         if (likely(hit)) {
            #[             ENTRY($tname)* default_entry = ${tname}_get_default_entry(STDPARAMS_IN);
            #[             hit = entry != default_entry;
            #}         }

            #{         #ifdef T4P4S_DEBUG
            #[             ${tname}_apply_show_hit_with_key(hit, entry  KEYTXTPARAM_IN, STDPARAMS_IN);
            #}         #endif

            #{         #ifdef T4P4S_STATS
            #[             t4p4s_stats_global.T4STAT(table,hit,$tname) = hit || t4p4s_stats_global.T4STAT(table,hit,$tname);
            #[             t4p4s_stats_global.T4STAT(table,miss,$tname) = !hit || t4p4s_stats_global.T4STAT(table,miss,$tname);
            #[             t4p4s_stats_per_packet.T4STAT(table,hit,$tname) = hit || t4p4s_stats_per_packet.T4STAT(table,hit,$tname);
            #[             t4p4s_stats_per_packet.T4STAT(table,miss,$tname) = !hit || t4p4s_stats_per_packet.T4STAT(table,miss,$tname);
            #}         #endif

        if len(table.direct_meters + table.direct_counters) > 0:
            #[         if (likely(hit))    ${tname}_apply_smems(STDPARAMS_IN);

        #[         ${tname}_stats(entry->id, STDPARAMS_IN);

        # ACTIONS
        if len(table.actions) == 1:
            ao = table.actions[0].action_object
            if len(ao.body.components) != 0:
                #[         action_code_${ao.name}(entry->params.${ao.name}_params, SHORT_STDPARAMS_IN);
            #[         return (apply_result_t) { hit, action_${ao.name} };
        else:
            #{         switch (entry->id) {
            for action in table.actions.filter(lambda act: len(act.action_object.body.components) != 0):
                ao = action.action_object
                #{           case action_${ao.name}:
                #[              action_code_${ao.name}(entry->params.${ao.name}_params, SHORT_STDPARAMS_IN);
                #}              return (apply_result_t) { hit, entry->id };
            if len(acts := table.actions.filter(lambda act: len(act.action_object.body.components) == 0)) > 0:
                cases = ' '.join(f'case action_{act.action_object.name}:' for act in acts)
                #{           ${cases}
                #[              return (apply_result_t) { hit, entry->id };
                #}
            #[           default: return (apply_result_t) {}; // unreachable
            #}         }

        #}     }
        #[

    ################################################################################

    for ctl, idx, comp in ctl_stages:
        #{ void control_stage_${ctl.name}_${idx}(control_locals_${ctl.name}_t* local_vars, STDPARAMS) {
        compiler_common.enclosing_control = ctl
        #= format_statement(comp, ctl)
        compiler_common.enclosing_control = None
        #} }
        #[
