#[ // SPDX-License-Identifier: Apache-2.0
#[ // Copyright 2020 Eotvos Lorand University, Budapest, Hungary

#[ #include "common.h"
#[ #include "dpdk_lib.h"

#[ extern char* action_canonical_names[];
#[ extern char* action_names[];

parser = hlir.parsers[0]

known_parser_state_names = ('start', 'accept', 'reject')
_, parser_state_names = zip(*sorted([(0 if s.name in known_parser_state_names else 1, s.name) for s in parser.states]))

#{ #ifdef T4P4S_STATS
#[     t4p4s_stats_t t4p4s_stats = {
for name in parser_state_names:
    #{         .parser_state__${name} = false,

#[

for table in hlir.tables:
    #[         .table_apply__${table.name} = false,

    if 'key' in table:
        #[         .table_hit__${table.name} = false,
        #[         .table_miss__${table.name} = false,
    else:
        #[         .table_used__${table.name} = false,

    for action_name in table.actions.map('expression.method.path.name'):
        #[         .table_action_used__${table.name}_${action_name} = false,

#}     };

#[ char stats_buf[16 * 1024];
#[ int stats_counter;

#{ void t4p4s_print_stats_parser_states(bool is_on) {
#[     sprintf(stats_buf, "");
#[     char* printout = stats_buf;

#[     stats_counter = 0;
#[     bool is_used;
#[     bool cond;
for name in parser_state_names:
    #[     is_used = t4p4s_stats.parser_state__${name};
    #[     cond = !(is_used ^ is_on);
    #{     if (cond) {
    #[         printout += sprintf(printout, "$$[parserstate]{name}, ");
    #[         ++stats_counter;
    #}     }

#[         if (stats_counter == 0 && !is_on)    return;
#[         // do not complain if only "reject" is unused
#[         if (stats_counter == 1 && !is_on && !t4p4s_stats.parser_state__reject)    return;
#[         debug("    - %2d %s parser states: %s\n", stats_counter, is_on ? "used" : "unused", stats_buf);
#} }

#[ enum t4p4s_table_stat_e { T4TABLE_APPLIED };

#{ void t4p4s_print_stats_tables(bool is_on, bool hidden, enum t4p4s_table_stat_e stat) {
#[     sprintf(stats_buf, "");
#[     char* printout = stats_buf;

#[     stats_counter = 0;
#[     bool is_used;
#[     bool is_hidden;
#[     bool cond;
for table in hlir.tables:
    #[     is_used = t4p4s_stats.table_apply__${table.name};
    #[     is_hidden  = ${"true" if table.is_hidden else "false"};
    #[     cond = !(is_used ^ is_on) && !(hidden ^ is_hidden);
    #{     if (cond) {
    #[         printout += sprintf(printout, "$$[table]{table.name}, ");
    #[         ++stats_counter;
    #}     }

#[         if (stats_counter == 0 && !is_on)    return;
#[         debug("    - %2d %s %stables: %s\n", stats_counter, is_on ? "applied" : "unapplied", hidden ? "hidden " : "", stats_buf);
#} }

#{ void t4p4s_print_stats_table_actions(bool is_on, bool hidden, enum t4p4s_table_stat_e stat) {
#[     sprintf(stats_buf, "");
#[     char* printout = stats_buf;

#[     stats_counter = 0;
#[     bool is_used;
#[     bool is_hidden;
#[     bool cond;
for table in hlir.tables:
    for action_name in table.actions.map('expression.method.path.name'):
        #[     is_used = t4p4s_stats.table_action_used__${table.name}_${action_name};
        #[     is_hidden  = ${"true" if table.is_hidden else "false"};
        #[     cond = !(is_used ^ is_on) && !(hidden ^ is_hidden);
        #{     if (cond) {
        #{         for (int i = 0; ; ++i) {
        #[             // note: looking up canonical name by "usual" name
        #{             if (!strcmp(action_names[i], "${action_name}")) {
        #[                 printout += sprintf(printout, T4LIT(%s,action) "@$$[table]{table.canonical_name}, ", action_canonical_names[i]);
        #[                 ++stats_counter;
        #[                 break; // the action is found, the lookup is done
        #}             }
        #}         }
        #}     }

#[         if (stats_counter == 0 && !is_on)    return;
#[         debug("    - %2d %s %sactions: %s\n", stats_counter, is_on ? "applied" : "unapplied", hidden ? "hidden " : "", stats_buf);
#} }

#{     void t4p4s_print_stats() {
#[         debug("Statistics:\n");
#[         t4p4s_print_stats_parser_states(true);

#[         t4p4s_print_stats_tables(true, false, T4TABLE_APPLIED);
#[         t4p4s_print_stats_tables(true, true, T4TABLE_APPLIED);

#[         t4p4s_print_stats_table_actions(true, false, T4TABLE_APPLIED);
#[         // hidden tables have exactly one action
#[         // t4p4s_print_stats_table_actions(true, true, T4TABLE_APPLIED);


#[         // "off" stats
#[         debug("\n");

#[         t4p4s_print_stats_parser_states(false);

#[         t4p4s_print_stats_tables(false, false, T4TABLE_APPLIED);
#[         t4p4s_print_stats_tables(false, true, T4TABLE_APPLIED);

#[         t4p4s_print_stats_table_actions(false, false, T4TABLE_APPLIED);
#[         // hidden tables have exactly one action
#[         // t4p4s_print_stats_table_actions(false, true, T4TABLE_APPLIED);

#}     }

#[ #else
#[ void t4p4s_print_stats() { /* do nothing */ }
#} #endif
