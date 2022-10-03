#[ // SPDX-License-Identifier: Apache-2.0
#[ // Copyright 2020 Eotvos Lorand University, Budapest, Hungary

from compiler_common import unique_everseen, to_c_bool

#[ #include "common.h"
#[ #include "dpdk_lib.h"

#[ extern char* action_short_names[];
#[ extern char* action_names[];

parser = hlir.parsers[0]

known_parser_state_names = ('start', 'accept', 'reject')
_, parser_state_names = zip(*sorted((0 if s.name in known_parser_state_names else 1, s.name) for s in parser.states))

#{ #ifndef T4P4S_STATS
#[     void t4p4s_print_global_stats()     { /* do nothing */ }
#[     void t4p4s_print_per_packet_stats() { /* do nothing */ }
#[     void t4p4s_init_global_stats()      { /* do nothing */ }
#[     void t4p4s_init_per_packet_stats()  { /* do nothing */ }
#[     void print_packet_stats(LCPARAMS)    { /* do nothing */ }
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
    #[     t4p4s_stats->T4STAT(parser,state,${name}) = false;

for table in hlir.tables:
    #[     t4p4s_stats->T4STAT(table,apply,${table.name}) = false;

    if 'key' in table:
        #[     t4p4s_stats->T4STAT(table,hit,${table.name}) = false;
        #[     t4p4s_stats->T4STAT(table,miss,${table.name}) = false;
    else:
        #[     t4p4s_stats->T4STAT(table,used,${table.name}) = false;

    for action_name in table.actions.map('expression.method.path.name'):
        #[     t4p4s_stats->T4STAT(action,${table.name},${action_name}) = false;
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
    #[     is_used = t4p4s_stats->T4STAT(parser,state,${name});
    #[     cond = !(is_used ^ is_on);
    #{     if (cond) {
    #[         printout += sprintf(printout, T4LIT($name,parserstate) ", ");
    #[         ++stats_counter;
    #}     }

#[     if (stats_counter == 0 && !is_on)    return;
#[     // do not complain if only "reject" is unused
#[     if (stats_counter == 1 && !is_on && !t4p4s_stats->T4STAT(parser,state,reject))    return;
#[     if (is_global) {
#[         debug("- %4d %s parser states: %s\n", stats_counter, is_on ? "used" : "unused", stats_buf);
#[     } else {
#[         debug("- parser states: %s\n", stats_buf);
#[     }
#} }

#[ enum t4p4s_table_stat_e { T4TABLE_APPLIED };

#{ void print_part(char** printout_ptr, const char*const table_short_name, bool is_on, bool hidden, bool is_table_hit, bool is_table_miss, bool is_table_applied, bool is_hidden) {
#[     bool is_hit  = (is_on == is_table_hit);
#[     bool is_miss = (is_on == is_table_miss);
#[     bool is_used = hidden ? is_table_applied : !(is_on ^ (is_hit || is_miss));
#[     bool cond = !(is_used ^ is_on) && !(hidden ^ is_hidden);
#[     if (!cond)   return;

#{     if (hidden || !is_on) {
#[         *printout_ptr += sprintf(*printout_ptr, T4LIT(%s,table) ", ", table_short_name);
#[     } else {
#[         *printout_ptr += sprintf(*printout_ptr, T4LIT(%s,table) "[%s%s], ", table_short_name, is_hit ? "hit" : "", is_miss ? "miss" : "");
#}     }
#[     ++stats_counter;
#} }
#[

#{ void t4p4s_print_stats_tables(bool is_on, bool hidden, enum t4p4s_table_stat_e stat, t4p4s_stats_t* t4p4s_stats, bool is_global) {
#[     sprintf(stats_buf, "%s", "");
#[     char* printout = stats_buf;

#[     stats_counter = 0;
for table in sorted(hlir.tables, key=lambda table: table.short_name):
    name = table.name
    #[ print_part(&printout, "${table.short_name}", is_on, hidden,
    #[            t4p4s_stats->T4STAT(table,hit,$name), t4p4s_stats->T4STAT(table,miss,$name), t4p4s_stats->T4STAT(table,apply,$name), ${to_c_bool(table.is_hidden)});

#[     if (stats_counter == 0 && !is_on)    return;
#[     if (is_global) {
#[         debug("- %4d %s %stables: %s\n", stats_counter, is_on ? "applied" : "unapplied", hidden ? "hidden " : "", stats_buf);
#[     } else {
#[         debug("- tables: %s\n", stats_buf);
#[     }
#} }
#[

#[ #define NO_ACTION_NAME ".NoAction"


#{ void t4p4s_print_stats_action(bool is_used, bool is_on, bool real_action, bool is_real_action, char** printout_ptr, int action_idx) {
#[     if ((is_used ^ is_on) || (real_action ^ is_real_action))   return;
#[     *printout_ptr += sprintf(*printout_ptr, "%s" T4LIT(%s,action), name_counter > 0 ? ", " : "", real_action ? action_short_names[action_idx] : "");
#[     ++name_counter;
#[     ++stats_counter;
#} }
#[


ta_reorder = {(t, a): idx for idx, (t, a) in enumerate((t,a) for t in hlir.tables for a in unique_everseen(t.actions))}

for idx, (table, action) in enumerate((t, a) for t in sorted(hlir.tables, key=lambda table: table.short_name) for a in sorted(unique_everseen(t.actions), key=lambda a: a.action_object.short_name)):
    ao = action.action_object
    action_idx = ta_reorder[(table, action)]

    is_hidden_ok = "" if table.is_hidden else "!"

    #{ void t4p4s_print_stats_${table.name}_${ao.name}(char** printout_ptr, bool is_on, bool real_action, t4p4s_stats_t* t4p4s_stats) {
    #[     bool is_used         = t4p4s_stats->T4STAT(action,${table.name},${ao.name});
    #[     bool is_real_action  = strcmp(action_short_names[$action_idx], NO_ACTION_NAME);
    #[     t4p4s_print_stats_action(is_used, is_on, real_action, is_real_action, printout_ptr, $action_idx);
    #} }
    #[

#{ void print_table_name(char** printout_ptr, const char*const table_short_name, bool real_action) {
#[     if (name_counter == 0)   return;

#[     if (real_action)    *printout_ptr += sprintf(*printout_ptr, "         " T4LIT(%s,table) ": %s\n", table_short_name, name_buf);
#[     else                *printout_ptr += sprintf(*printout_ptr, T4LIT(%s,table) ", ", table_short_name);
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
    is_hidden_ok = "" if table.is_hidden else "!"
    for action_name in unique_everseen(table.actions.map('expression.method.path.name')):
        #[     if (${is_hidden_ok}hidden)   t4p4s_print_stats_${table.name}_${action_name}(&printout_name, is_on, real_action, t4p4s_stats);
    #[     print_table_name(&printout, "${table.short_name}", real_action);
    #[

#[     if (stats_counter == 0 && !is_on)    return;
#{     if (is_global) {
#[         debug("- %4d %s %s%s:%s%s%s", stats_counter, is_on ? "applied" : "unapplied", hidden ? "hidden " : "", action_type_txt, real_action ? "\n" : " ", stats_buf, real_action ? "" : "\n");
#[     } else {
#[         debug("- %s:%s%s%s", action_type_txt, real_action ? "\n" : " ", stats_buf, real_action ? "" : "\n");
#}     }
#} }
#[

#{     #ifdef T4P4S_DEBUG
#[         extern volatile int packet_with_error_counter;
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


#{ void print_packet_stats(LCPARAMS) {
#[     COUNTER_ECHO(lcdata->conf->processed_packet_num,"   :: Processed packet count: %d\n");
#[     COUNTER_ECHO(lcdata->conf->doing_crypto_packet,"   :: Crypto packets in progress: %d\n");
#[     COUNTER_ECHO(lcdata->conf->fwd_packet,"   :: Forwarded packet without encrypt: %d\n");
#[     #if defined ASYNC_MODE && ASYNC_MODE != ASYNC_OFF
#[          COUNTER_ECHO(lcdata->conf->sent_to_crypto_packet,"   :: Sent to crypto packet: %d\n");
#[          COUNTER_ECHO(lcdata->conf->async_packet,"   :: Async handled packet: %d\n");
#[          COUNTER_ECHO(lcdata->conf->async_drop_counter,"   :: Dropped async packet: %d\n");
#[     #endif
#} }
#[

#} #endif
#[
