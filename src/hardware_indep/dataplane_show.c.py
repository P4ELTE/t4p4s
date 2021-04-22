# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_declaration, format_statement, format_expr, format_type, gen_format_type, get_method_call_env
from compiler_log_warnings_errors import addError, addWarning
from compiler_common import types, generate_var_name, get_hdrfld_name, unique_everseen

#[ #include "dataplane_impl.h"

table_infos = [(table, table.short_name + ("/keyless" if table.key_length_bits == 0 else "") + ("/hidden" if table.is_hidden else "")) for table in hlir.tables]

################################################################################

#{ #ifdef T4P4S_DEBUG

#{     void show_params_by_action_id(char* out, int table_id, int action_id, const void* entry) {
for table in hlir.tables:
    if 'key' not in table or table.key_length_bits == 0:
        #[         if (table_id == TABLE_${table.name}) { sprintf(out, "%s", ""); return; }
        continue

    #{          if (table_id == TABLE_${table.name}) {
    for dbg_action in table.actions:
        aoname = dbg_action.action_object.name
        #{              if (action_id == action_${aoname}) {
        #[                  ${table.name}_show_params_${aoname}(out, (action_${aoname}_params_t*)entry);
        #[                  return;
        #}              }
    #}          }
#}      }


for table in hlir.tables:
    if 'key' not in table or table.key_length_bits == 0:
        continue

    for dbg_action in table.actions:
        aoname = dbg_action.action_object.name
        #{     void ${table.name}_show_params_${aoname}(char* out, const action_${aoname}_params_t* actpar) {
        params = dbg_action.expression.method.type.parameters.parameters

        def make_value(value):
            is_hex = value.base == 16
            split_places = 4 if is_hex else 3

            prefix = '0x' if is_hex else ''
            val = f'{value.value:x}' if is_hex else f'{value.value}'
            val = '_'.join(val[::-1][i:i+split_places] for i in range(0, len(val), split_places))[::-1]
            return f'{prefix}{val}'

        fmt_long_param = lambda sz: ('%02x%02x ' * (sz//2) + ('%02x' if sz%2 == 1 else '')).strip()

        param_fmts = (f'" T4LIT(%d) "=" T4LIT(%0{(sz+3)//4}x,bytes) "' if sz <= 32 else f'(" T4LIT({fmt_long_param((sz+7)//8)},bytes) ")' for param in params for sz in [param.urtype.size])
        params_str = ", ".join((f'%s" T4LIT({param.name},field) "/" T4LIT({param.urtype.size}b) "={fmt}' for param, fmt in zip(params, param_fmts)))
        if params_str != "":
            params_str = f'({params_str})'

        #[         sprintf(out, "${params_str}%s",
        for param in params:
            bytesz = (param.urtype.size+7)//8
            if param.urtype.size <= 32:
                bytesz_aligned = 1 if bytesz == 1 else 2 if bytesz == 2 else 4
                converter = '' if bytesz == 1 else f't4p4s2net_{bytesz_aligned}'

                #[               ${bytesz} > 1 ? "§" : "", // maybe cpu->BE conversion
                #[               $converter(actpar->${param.name}), // decimal
                #[               $converter(actpar->${param.name}), // hex
            else:
                #[               "", // no cpu->BE conversion
                for i in range(bytesz):
                    #[               (actpar->${param.name})[$i],
        #[         "");
        #}     }
        #[

#{     void apply_show_hit_with_key_msg(uint8_t** key, bool hit, int key_size, int key_length_bytes, const char* matchtype_name, const char* action_short_name, const char* params_txt, const char* table_info, STDPARAMS) {
#[         dbg_bytes(key, key_size,
#[                   " %s Lookup on " T4LIT(%s,table) "/" T4LIT(%s) "/" T4LIT(%dB) ": $$[action]{}{%s}%s%s <- %s ",
#[                   hit ? T4LIT(++++,success) : T4LIT(XXXX,status),
#[                   table_info,
#[                   matchtype_name,
#[                   key_length_bytes,
#[                   action_short_name,
#[                   params_txt,
#[                   hit ? "" : " (default)",
#[                   hit ? T4LIT(hit,success) : T4LIT(miss,status)
#[                   );
#}     }

for table, table_info in table_infos:
    if 'key' not in table or table.key_length_bits == 0:
        continue

    #{     void ${table.name}_apply_show_hit_with_key(uint8_t* key[${table.key_length_bytes}], bool hit, const table_entry_${table.name}_t* entry, STDPARAMS) {
    #[         char params_txt[1024];
    for dbg_action in table.actions:
        aoname = dbg_action.action_object.name
        dbg_action_name = dbg_action.expression.method.path.name
        #{         if (!strcmp("${dbg_action_name}", action_names[entry->action.action_id])) {
        #[             ${table.name}_show_params_${aoname}(params_txt, &(entry->action.${aoname}_params));
        #[             apply_show_hit_with_key_msg(key, hit, table_config[TABLE_${table.name}].entry.key_size, ${table.key_length_bytes}, "${table.matchType.name}", action_short_names[entry->action.action_id], params_txt, "${table_info}", STDPARAMS_IN);
        #}         }
    #}     }
    #[
#} #endif

for table, table_info in table_infos:
    if 'key' in table and table.key_length_bits > 0:
        continue

    #{ void ${table.name}_apply_show_hit(int action_id, STDPARAMS) {
    if 'key' not in table:
        #[     debug(" :::: Lookup on " T4LIT(${table_info},table) ", default action is " T4LIT(%s,action) "\n", action_short_names[action_id]);
    elif table.key_length_bits == 0:
        if table.is_hidden or len(table.actions) == 1:
            #[     debug(" ~~~~ Action $$[action]{}{%s}\n", action_short_names[action_id]);
        else:
            #[     debug(" " T4LIT(XXXX,status) " Lookup on $$[table]{table_info}: $$[action]{}{%s} (default)\n", action_short_names[action_id]);
    #} }
    #[
