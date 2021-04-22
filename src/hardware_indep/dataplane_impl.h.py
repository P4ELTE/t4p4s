# SPDX-License-Identifier: Apache-2.0
# Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#[ #pragma once

from utils.codegen import format_declaration, format_statement, format_expr, format_type, gen_format_type, get_method_call_env
from compiler_log_warnings_errors import addError, addWarning
from compiler_common import types, generate_var_name, get_hdrfld_name, unique_everseen

#[ #include <stdlib.h>
#[ #include <string.h>
#[ #include <stdbool.h>
#[ #include <netinet/in.h>

#[ #include "dpdk_lib.h"
#[ #include "actions.h"
#[ #include "backend.h"
#[ #include "util_debug.h"
#[ #include "tables.h"
#[ #include "gen_include.h"

#{ #ifdef T4P4S_P4RT
#[     #include "PI/proto/pi_server.h"
#[     #include "p4rt/device_mgr.h"
#[     extern device_mgr_t *dev_mgr_ptr;
#} #endif

#[ extern ctrl_plane_backend bg;
#[ extern char* action_short_names[];
#[ extern char* action_names[];

#[ void parse_packet(STDPARAMS);
#[ void increase_counter(int counterid, int index);
#[ void set_handle_packet_metadata(packet_descriptor_t* pd, uint32_t portid);

#{ #ifdef T4P4S_STATS
#[     extern t4p4s_stats_t t4p4s_stats_global;
#[     extern t4p4s_stats_t t4p4s_stats_per_packet;
#} #endif

#[ void update_packet(STDPARAMS);

table_infos = [(table, table.short_name + ("/keyless" if table.key_length_bits == 0 else "") + ("/hidden" if table.is_hidden else "")) for table in hlir.tables]

for table in hlir.tables:
    if 'key' not in table or table.key_length_bits == 0:
        continue

    #[ void table_${table.name}_key(packet_descriptor_t* pd, uint8_t* key);

for table, table_info in table_infos:
    #[ void ${table.name}_stats(int action_id, STDPARAMS);

for table, table_info in table_infos:
    #[ table_entry_${table.name}_t* ${table.name}_get_default_entry(STDPARAMS);

for table, table_info in table_infos:
    if len(table.direct_meters + table.direct_counters) == 0:
        continue

    #[ void ${table.name}_apply_smems(STDPARAMS);


packet_name = hlir.news.main.type.baseType.type_ref.name
pipeline_elements = hlir.news.main.arguments

#{ typedef struct {
#[     bool hit;
#[     int action_run;
#} } apply_result_t;
#[

for ctl in hlir.controls:
    #[ void control_${ctl.name}(STDPARAMS);
#[

for ctl in hlir.controls:
    for t in ctl.controlLocals['P4Table']:
        #[ apply_result_t ${t.name}_apply(STDPARAMS);

for table, table_info in table_infos:
    if len(table.direct_meters + table.direct_counters) == 0:
        continue

    #[ void ${table.name}_apply_smems(STDPARAMS);

#{ #ifdef T4P4S_DEBUG
#[     void show_params_by_action_id(char* out, int table_id, int action_id, const void* entry);

for table in hlir.tables:
    if 'key' not in table or table.key_length_bits == 0:
        continue

    #[     void ${table.name}_apply_show_hit_with_key(uint8_t* key[${table.key_length_bytes}], bool hit, const table_entry_${table.name}_t* entry, STDPARAMS);

    for dbg_action in table.actions:
        aoname = dbg_action.action_object.name
        #[     void ${table.name}_show_params_${aoname}(char* out, const action_${aoname}_params_t* actpar);

#} #endif

for table, table_info in table_infos:
    if 'key' in table and table.key_length_bits > 0:
        continue

    #[ void ${table.name}_apply_show_hit(int action_id, STDPARAMS);
