#[ // SPDX-License-Identifier: Apache-2.0
#[ // Copyright 2020 Eotvos Lorand University, Budapest, Hungary

#[ #include "common.h"
#[ #include "dpdk_lib.h"

#[ extern char* action_short_names[];
#[ extern char* action_names[];

parser = hlir.parsers[0]

known_parser_state_names = ('start', 'accept', 'reject')
_, parser_state_names = zip(*sorted([(0 if s.name in known_parser_state_names else 1, s.name) for s in parser.states]))

#{ #ifdef T4P4S_STATS
#[ t4p4s_stats_t t4p4s_stats_global;
#[ t4p4s_stats_t t4p4s_stats_per_packet;

#[ char stats_buf[16 * 1024];
#[ int stats_counter;

#{ void t4p4s_init_stats(t4p4s_stats_t* t4p4s_stats) {
for name in parser_state_names:
    #{     t4p4s_stats->parser_state__${name} = false;

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
#[	t4p4s_init_stats(&t4p4s_stats_global);
#}	}
#{ void t4p4s_init_per_packet_stats() { 
#[	t4p4s_init_stats(&t4p4s_stats_per_packet); 
#}	}

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
    #[         printout += sprintf(printout, "$$[parserstate]{name}, ");
    #[         ++stats_counter;
    #}     }

#[         if (stats_counter == 0 && !is_on)    return;
#[         // do not complain if only "reject" is unused
#[         if (stats_counter == 1 && !is_on && !t4p4s_stats->parser_state__reject)    return;
#[		   if (is_global) {
#[             debug("    - %2d %s parser states: %s\n", stats_counter, is_on ? "used" : "unused", stats_buf);
#[         } else {
#[             debug("    - parser states: %s\n", stats_buf);
#[		   }
#} }

#[ enum t4p4s_table_stat_e { T4TABLE_APPLIED };

#{ void t4p4s_print_stats_tables(bool is_on, bool hidden, enum t4p4s_table_stat_e stat, t4p4s_stats_t* t4p4s_stats, bool is_global) {
#[     sprintf(stats_buf, "%s", "");
#[     char* printout = stats_buf;

#[     stats_counter = 0;
#[     bool is_used;
#[     bool is_hidden;
#[     bool cond;
for table in hlir.tables:
    #[     is_used = t4p4s_stats->table_apply__${table.name};
    #[     is_hidden  = ${"true" if table.is_hidden else "false"};
    #[     cond = !(is_used ^ is_on) && !(hidden ^ is_hidden);
    #{     if (cond) {
    #[         printout += sprintf(printout, "$$[table]{table.name}, ");
    #[         ++stats_counter;
    #}     }

#[         if (stats_counter == 0 && !is_on)    return;
#[		   if (is_global) {
#[         	   debug("    - %2d %s %stables: %s\n", stats_counter, is_on ? "applied" : "unapplied", hidden ? "hidden " : "", stats_buf);
#[		   } else {
#[         	   debug("    - tables: %s\n", stats_buf);
#[		   }
#} }

#{ void t4p4s_print_stats_table_actions(bool is_on, bool hidden, enum t4p4s_table_stat_e stat, t4p4s_stats_t* t4p4s_stats, bool is_global) {
#[     sprintf(stats_buf, "%s", "");
#[     char* printout = stats_buf;

#[     stats_counter = 0;
#[     bool is_used;
#[     bool is_hidden;
#[     bool cond;
for table in hlir.tables:
    for action_name in table.actions.map('expression.method.path.name'):
        #[     is_used = t4p4s_stats->table_action_used__${table.name}_${action_name};
        #[     is_hidden  = ${"true" if table.is_hidden else "false"};
        #[     cond = !(is_used ^ is_on) && !(hidden ^ is_hidden);
        #{     if (cond) {
        #{         for (int i = 0; ; ++i) {
        #[             // note: looking up canonical name by "usual" name
        #{             if (!strcmp(action_names[i], "${action_name}")) {
        #[                 printout += sprintf(printout, T4LIT(%s,action) "@$$[table]{table.short_name}, ", action_short_names[i]);
        #[                 ++stats_counter;
        #[                 break; // the action is found, the lookup is done
        #}             }
        #}         }
        #}     }

#[         if (stats_counter == 0 && !is_on)    return;
#[		   if (is_global) {
#[             debug("    - %2d %s %sactions: %s\n", stats_counter, is_on ? "applied" : "unapplied", hidden ? "hidden " : "", stats_buf);
#[		   } else {
#[             debug("    - actions: %s\n", stats_buf);
#[		   }
#} }

#{     #ifdef T4P4S_DEBUG
#[         extern int packet_with_error_counter;
#[         extern volatile int packet_counter;
#}     #endif

#{     void t4p4s_print_stats_error_packets() {
#{         #ifdef T4P4S_DEBUG
#[             int all  = packet_counter;
#[             int errs = packet_with_error_counter;
#{             if (errs == 0) {
#[                 debug("    - " T4LIT(%2d,success) " OK packet%s\n", all, all != 1 ? "s" : "");
#[             } else {
#[                 debug("    - " T4LIT(%2d,error) " error%s in packet processing, " T4LIT(%2d,success) " OK packet%s (" T4LIT(%2d) " packet%s in total)\n",
#[                       errs, errs != 1 ? "s" : "", all - errs, all - errs != 1 ? "s" : "", all, all != 1 ? "s" : "");
#}             }
#}         #endif
#}     }

#{     void t4p4s_print_global_stats() {
#[         debug("\n");
#[		   debug("Overall statistics:\n");
#[         t4p4s_print_stats_error_packets();
#[         t4p4s_print_stats_parser_states(true, &t4p4s_stats_global, true);

#[         t4p4s_print_stats_tables(true, false, T4TABLE_APPLIED, &t4p4s_stats_global, true);
#[         t4p4s_print_stats_tables(true, true, T4TABLE_APPLIED, &t4p4s_stats_global, true);

#[         t4p4s_print_stats_table_actions(true, false, T4TABLE_APPLIED, &t4p4s_stats_global, true);
#[         // hidden tables have exactly one action
#[         // t4p4s_print_stats_table_actions(true, true, T4TABLE_APPLIED, &t4p4s_stats_global, true);


#[         // "off" stats
#[         debug("\n");

#[         t4p4s_print_stats_parser_states(false, &t4p4s_stats_global, true);

#[         t4p4s_print_stats_tables(false, false, T4TABLE_APPLIED, &t4p4s_stats_global, true);
#[         t4p4s_print_stats_tables(false, true, T4TABLE_APPLIED, &t4p4s_stats_global, true);

#[         t4p4s_print_stats_table_actions(false, false, T4TABLE_APPLIED, &t4p4s_stats_global, true);
#[         // hidden tables have exactly one action
#[         // t4p4s_print_stats_table_actions(false, true, T4TABLE_APPLIED, &t4p4s_stats_global, true);

#}     }

#{     void t4p4s_print_per_packet_stats() {
#[         debug("Per packet statistics:\n");
#[         t4p4s_print_stats_parser_states(true, &t4p4s_stats_per_packet, false);

#[         t4p4s_print_stats_tables(true, false, T4TABLE_APPLIED, &t4p4s_stats_per_packet, false);
#[         //t4p4s_print_stats_tables(true, true, T4TABLE_APPLIED, &t4p4s_stats_per_packet, false);

#[         t4p4s_print_stats_table_actions(true, false, T4TABLE_APPLIED, &t4p4s_stats_per_packet, false);

#}     }

#[ #else
#[ void t4p4s_print_global_stats() { /* do nothing */ }
#[ void t4p4s_print_per_packet_stats() { /* do nothing */ }
#[ void t4p4s_init_global_stats()  { /* do nothing */ }
#[ void t4p4s_init_per_packet_stats()  { /* do nothing */ }
#} #endif
