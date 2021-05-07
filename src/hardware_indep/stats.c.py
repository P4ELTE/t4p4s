#[ // SPDX-License-Identifier: Apache-2.0
#[ // Copyright 2020 Eotvos Lorand University, Budapest, Hungary

from compiler_common import unique_everseen

#[ #include "common.h"
#[ #include "dpdk_lib.h"

#[ extern char* action_short_names[];
#[ extern char* action_names[];

parser = hlir.parsers[0]

known_parser_state_names = ('start', 'accept', 'reject')
_, parser_state_names = zip(*sorted((0 if s.name in known_parser_state_names else 1, s.name) for s in parser.states))

#{ #ifndef T4P4S_STATS
#[     void t4p4s_print_global_stats() { /* do nothing */ }
#[     void t4p4s_print_per_packet_stats() { /* do nothing */ }
#[     void t4p4s_init_global_stats()  { /* do nothing */ }
#[     void t4p4s_init_per_packet_stats()  { /* do nothing */ }
#[ #else
#[
#[ t4p4s_stats_t t4p4s_stats_global;
#[ t4p4s_stats_t t4p4s_stats_per_packet;

#[ char name_buf[128 * 1024];
#[ int name_counter;

#[ char stats_buf[128 * 1024];
#[ int stats_counter;

#{ void t4p4s_init_stats(t4p4s_stats_t* t4p4s_stats) {
for name in parser_state_names:
    #[     t4p4s_stats->parser_state__${name} = false;

for table in hlir.tables:
    #[     t4p4s_stats->table_apply__${table.name} = false;

    if 'key' in table:
        #[     t4p4s_stats->table_hit__${table.name} = false;
        #[     t4p4s_stats->table_miss__${table.name} = false;
    else:
        #[     t4p4s_stats->table_used__${table.name} = false;

    for action_name in table.actions.map('expression.method.path.name'):
        #[     t4p4s_stats->table_action_used__${table.name}_${action_name} = false;
#} }

#{ void t4p4s_init_global_stats() {
#[     t4p4s_init_stats(&t4p4s_stats_global);
#} }
#[

#{ void t4p4s_init_per_packet_stats() {
#[     t4p4s_init_stats(&t4p4s_stats_per_packet);
#} }
#[

#{ void t4p4s_print_stats_parser_states(bool is_on, t4p4s_stats_t* t4p4s_stats, bool is_global) {
#[     sprintf(stats_buf, "%s", "");
#[     char* printout = stats_buf;

#[     stats_counter = 0;
#[     bool is_used;
#[     bool cond;
for name in parser_state_names:
    #[     is_used = t4p4s_stats->parser_state__${name};
    #[     cond = !(is_used ^ is_on);
    #{     if (cond) {
    #[         printout += sprintf(printout, T4LIT($name,parserstate) ", ");
    #[         ++stats_counter;
    #}     }

#[     if (stats_counter == 0 && !is_on)    return;
#[     // do not complain if only "reject" is unused
#[     if (stats_counter == 1 && !is_on && !t4p4s_stats->parser_state__reject)    return;
#[     if (is_global) {
#[         debug("- %4d %s parser states: %s\n", stats_counter, is_on ? "used" : "unused", stats_buf);
#[     } else {
#[         debug("- parser states: %s\n", stats_buf);
#[     }
#} }

#[ enum t4p4s_table_stat_e { T4TABLE_APPLIED };

#{ void t4p4s_print_stats_tables(bool is_on, bool hidden, enum t4p4s_table_stat_e stat, t4p4s_stats_t* t4p4s_stats, bool is_global) {
#[     sprintf(stats_buf, "%s", "");
#[     char* printout = stats_buf;

#[     stats_counter = 0;
#[     bool is_used;
#[     bool is_hit;
#[     bool is_miss;
#[     bool is_hidden;
#[     bool cond;
for table in sorted(hlir.tables, key=lambda table: table.short_name):
    #[     is_hit  = (is_on && t4p4s_stats->table_hit__${table.name}) || (!is_on && !t4p4s_stats->table_hit__${table.name});
    #[     is_miss = (is_on && t4p4s_stats->table_miss__${table.name}) || (!is_on && !t4p4s_stats->table_miss__${table.name});
    #[     if (hidden) {
    #[         is_used = t4p4s_stats->table_apply__${table.name};
    #[     } else {
    #[         is_used = !(is_on ^ (is_hit || is_miss));
    #[     }
    #[     is_hidden  = ${"true" if table.is_hidden else "false"};
    #[     cond = !(is_used ^ is_on) && !(hidden ^ is_hidden);
    #{     if (cond) {
    #[         if (hidden || !is_on) {
    #[             printout += sprintf(printout, "$$[table]{table.name}, ");
    #[         } else {
    #[             printout += sprintf(printout, "$$[table]{table.name}[%s%s], ", is_hit ? "hit": "", is_miss ? "miss": "");
    #[         }
    #[         ++stats_counter;
    #}     }
    #[

#[         if (stats_counter == 0 && !is_on)    return;
#[         if (is_global) {
#[             debug("- %4d %s %stables: %s\n", stats_counter, is_on ? "applied" : "unapplied", hidden ? "hidden " : "", stats_buf);
#[         } else {
#[             debug("- tables: %s\n", stats_buf);
#[         }
#} }
#[

#[ #define NO_ACTION_NAME ".NoAction"

ta_reorder = {(t, a): idx for idx, (t, a) in enumerate((t,a) for t in hlir.tables for a in unique_everseen(t.actions))}

for idx, (table, action) in enumerate((t, a) for t in sorted(hlir.tables, key=lambda table: table.short_name) for a in sorted(unique_everseen(t.actions), key=lambda a: a.action_object.short_name)):
    ao = action.action_object
    action_idx = ta_reorder[(table, action)]

    #{ void t4p4s_print_stats_${table.name}_${ao.name}(char** printout, bool is_on, bool hidden, bool real_action, t4p4s_stats_t* t4p4s_stats) {
    #[     bool is_used = t4p4s_stats->table_action_used__${table.name}_${ao.name};
    #[     bool is_hidden  = ${"true" if table.is_hidden else "false"};
    #[     bool is_real_action  = strcmp(action_short_names[$action_idx], NO_ACTION_NAME);
    #[     if (is_used ^ is_on)      return;
    #[     if (hidden ^ is_hidden)   return;
    #[     if (real_action ^ is_real_action)   return;
    #[     *printout += sprintf(*printout, "%s" T4LIT(%s,action), name_counter > 0 ? ", " : "", real_action ? action_short_names[$action_idx] : "");
    #[     ++name_counter;
    #[     ++stats_counter;
    #} }
    #[

#{ void t4p4s_print_stats_table_actions(bool is_on, bool hidden, bool real_action, const char* action_type_txt, enum t4p4s_table_stat_e stat, t4p4s_stats_t* t4p4s_stats, bool is_global) {
#[     sprintf(stats_buf, "%s", "");
#[     char* printout = stats_buf;
#[     char* printout_name;

#[     stats_counter = 0;
for table in sorted(hlir.tables, key=lambda table: table.short_name):
    #[     sprintf(name_buf, "%s", "");
    #[     printout_name = name_buf;

    #[     name_counter = 0;
    for action_name in table.actions.map('expression.method.path.name'):
        #[     t4p4s_print_stats_${table.name}_${action_name}(&printout_name, is_on, hidden, real_action, t4p4s_stats);

    #{     if (name_counter > 0) {
    #[         if (real_action)    printout += sprintf(printout, "         " T4LIT(%s,table) ": %s\n", "${table.short_name}", name_buf);
    #[         else                printout += sprintf(printout, T4LIT(%s,table) ", ", "${table.short_name}");
    #}     }

#[     if (stats_counter == 0 && !is_on)    return;
#{     if (is_global) {
#[         debug("- %4d %s %s%s:%s%s%s", stats_counter, is_on ? "applied" : "unapplied", hidden ? "hidden " : "", action_type_txt, real_action ? "\n" : " ", stats_buf, real_action ? "" : "\n");
#[     } else {
#[         debug("- %s:%s%s%s", action_type_txt, real_action ? "\n" : " ", stats_buf, real_action ? "" : "\n");
#}     }
#} }
#[

#{     #ifdef T4P4S_DEBUG
#[         extern int packet_with_error_counter;
#[         extern volatile int packet_counter;
#}     #endif
#[

#{     void t4p4s_print_stats_error_packets() {
#{         #ifdef T4P4S_DEBUG
#[             int all  = packet_counter;
#[             int errs = packet_with_error_counter;
#{             if (errs == 0) {
#[                 debug("- " T4LIT(%2d,success) " OK packet%s\n", all, all != 1 ? "s" : "");
#[             } else {
#[                 debug("- " T4LIT(%2d,error) " error%s in packet processing, " T4LIT(%2d,success) " OK packet%s (" T4LIT(%2d) " packet%s in total)\n",
#[                       errs, errs != 1 ? "s" : "", all - errs, all - errs != 1 ? "s" : "", all, all != 1 ? "s" : "");
#}             }
#}         #endif
#}     }

#{     void t4p4s_print_stats(bool is_on, t4p4s_stats_t* t4p4s_stats, bool is_global) {
#[         t4p4s_print_stats_parser_states(is_on, t4p4s_stats, is_global);

#[         t4p4s_print_stats_tables(is_on, false, T4TABLE_APPLIED, t4p4s_stats, is_global);
#{         if (is_global) {
#[             t4p4s_print_stats_tables(is_on, true, T4TABLE_APPLIED, t4p4s_stats, is_global);
#}         }

#[         t4p4s_print_stats_table_actions(is_on, false, true, "real actions", T4TABLE_APPLIED, t4p4s_stats, is_global);
#[         t4p4s_print_stats_table_actions(is_on, false, false, "no-actions", T4TABLE_APPLIED, t4p4s_stats, is_global);
#}     }
#[


#{     void t4p4s_print_global_stats() {
#[         debug("\n");
#[         debug("Overall statistics:\n");
#[         t4p4s_print_stats_error_packets();
#[         t4p4s_print_stats(true, &t4p4s_stats_global, true);
#[         debug("\n");
#[         t4p4s_print_stats(false, &t4p4s_stats_global, true);
#}     }
#[

#{     void t4p4s_print_per_packet_stats() {
#[         debug("Per packet statistics:\n");
#[         t4p4s_print_stats(true, &t4p4s_stats_per_packet, false);

#}     }

#{     bool t4p4s_check_stat_based_requirement(t4p4s_stats_t stat, t4p4s_controlflow_name_t requirement, bool positive) {
parser = hlir.parsers[0]
for s in parser.states:
    #[         if (requirement==req_parser_state__${s.name} && ((positive && stat.parser_state__${s.name}) || (!positive && !stat.parser_state__${s.name}))) {return true;}

#[

for table in hlir.tables:
    #[         if (requirement==req_table_apply__${table.name} && ((positive && stat.table_apply__${table.name}) || (!positive && !stat.table_apply__${table.name}))) {return true;}

    if 'key' in table:
        #[         if (requirement==req_table_hit__${table.name} && ((positive && stat.table_hit__${table.name}) || (!positive && !stat.table_hit__${table.name}))) {return true;}
        #[         if (requirement==req_table_miss__${table.name} && ((positive && stat.table_miss__${table.name}) || (!positive && !stat.table_miss__${table.name}))) {return true;}
    else:
        #[         if (requirement==req_table_used__${table.name} && ((positive && stat.table_used__${table.name}) || (!positive && !stat.table_used__${table.name}))) {return true;}


    for action_name in table.actions.map('expression.method.path.name'):
        #[         if (requirement==req_table_action_used__${table.name}_${action_name} && ((positive && stat.table_action_used__${table.name}_${action_name}) || (!positive && !stat.table_action_used__${table.name}_${action_name}))) {return true;}

#[         return false;
#}     }
#[

#{     bool check_controlflow_requirements(fake_cmd_t cmd) {
#[         bool ok = true;

#[         t4p4s_controlflow_name_t* name_require = cmd.require;

#{         while (*name_require > 0) {
#[             ok = ok && t4p4s_check_stat_based_requirement(t4p4s_stats_per_packet, *name_require, 1);
#[             ++name_require;
#}         }

#[         t4p4s_controlflow_name_t* name_forbid = cmd.forbid;

#{         while (*name_forbid > 0) {
#[             ok = ok && t4p4s_check_stat_based_requirement(t4p4s_stats_per_packet, *name_forbid, 0);
#[             ++name_forbid;
#}         }

#[         return ok;
#}    }

#} #endif
#[
