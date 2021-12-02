#[ // SPDX-License-Identifier: Apache-2.0
#[ // Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#[ #include <string.h>

#[ #include "common.h"
#[ #include "dpdk_lib.h"


#{ #ifndef T4P4S_STATS
#[ #else

#[ extern t4p4s_stats_t t4p4s_stats_per_packet;


reqconds = 'apply hit miss cond run parse NONE ERROR'.split(' ')

#{ typedef enum {
for cond in reqconds:
    #[     reqcond_${cond},
#} } reqcond_e;
#[

#{ reqcond_e get_reqcond(const char* name) {
for cond in reqconds:
    #[     if (!strcmp("$cond", name))    return reqcond_$cond;
#[     return reqcond_ERROR;
#} };
#[


#{ const char* get_reqcond_txt(reqcond_e reqcond) {
for cond in reqconds:
    #[     if (reqcond_$cond == reqcond)    return "$cond";
#[     return "unreachable code";
#} };
#[


#[ #define MAX_PART_COUNT 128
#[

#{ const char*const prefix_skip_char(const char*const txt, char c) {
#[     const char* ptr = txt;
#[     while (*ptr == c)  ++ptr;
#[     return ptr;
#} }
#[

#{ int split(char* parts[MAX_PART_COUNT], const char* reqs, const char* separator) {
#[     int count = 0;
#[     char* tmp = strdup(reqs);
#[     while ( (parts[count] = strsep(&tmp,separator)) != NULL )  ++count;
#[     free(tmp);
#[     return count;
#} }
#[


#{ bool check_table_name(const char*const table_name) {
for table in hlir.tables:
    #[     if (!strcmp("${table.short_name}", table_name))    return true;
#[     return false;
#} }
#[


#{ bool check_table_requirement(const char*const table_name, reqcond_e reqcond, bool on) {
#[     t4p4s_stats_t stat = t4p4s_stats_per_packet;
for table in hlir.tables:
    name = table.name
    sname = table.short_name

    conds = 'apply' + (' hit miss' if 'key' in table else ' used')
    for cond in conds.split(' '):
        #[     if (!strcmp("$sname", table_name) && reqcond == reqcond_$cond && (on == stat.T4STAT(table,${cond},$name)))   return true;
#[     return false;
#} }
#[

# TODO
#{ bool check_action_requirement(const char*const table_name, reqcond_e reqcond, bool on) {
#[     return false;
#} }
#[

# TODO
#{ bool check_parser_requirement(const char*const table_name, reqcond_e reqcond, bool on) {
#[     return false;
#} }
#[


#{ bool check_cond(const char*const cond) {
#[     const char*const ltrimmed_cond = prefix_skip_char(cond, ' ');
#[     bool on = true;

#[     bool result = true;
#[     reqcond_e reqcond = reqcond_NONE;
#[     char* parts[MAX_PART_COUNT];
#[     int part_count = split(parts, cond, " ");
#[     for (int i = 0; i < part_count; ++i) {
#[         if (strlen(parts[i]) == 0)    continue;
for cond in reqconds:
    #{         if (!strcmp("$cond", parts[i])) {
    #{             if (reqcond != reqcond_NONE) {
    #[                 debug("    " T4LIT(!,warning) " More than one requirement (" T4LIT($cond,warning) " and " T4LIT(%s,warning) ") found in condition " T4LIT(%s,warning) "\n",
    #[                       parts[i], ltrimmed_cond);
    #[                 result = false;
    #[                 goto free_mem; // breaking out of outermost loop
    #}             }
    #[             reqcond = get_reqcond(parts[i]);
    #[             continue;
    #}         }

#{         if (!strcmp("not", parts[i])) {
#{             if (!on) {
#[                 debug("    " T4LIT(!,warning) " Condition contains multiple negation: " T4LIT(%s,warning) "\n", ltrimmed_cond);
#[                 result = false;
#[                 goto free_mem; // breaking out of outermost loop
#}             }
#[             on = false;
#[             continue;
#}         }

#[         // we have found a table name

#{         if (reqcond == reqcond_NONE) {
#[             debug("    " T4LIT(!,warning) " Table " T4LIT(%s,table) " found, but no check (e.g. " T4LIT(hit) ") given in condition " T4LIT(%s,warning) "\n", parts[i], ltrimmed_cond);
#[             result = false;
#[             goto free_mem; // breaking out of outermost loop
#}         }

#[         result &= check_table_name(parts[i]);
#{         if (!result) {
#[             debug("    " T4LIT(!,warning) " Nonexistent table " T4LIT(%s,table) " given in condition " T4LIT(%s,warning) "\n", parts[i], ltrimmed_cond);
#[             result = false;
#[             goto free_mem; // breaking out of outermost loop
#}         }

#[         result &= check_table_requirement(parts[i], reqcond, on);
#{         if (!result) {
#[             debug("    " T4LIT(!,warning) " Requirement " T4LIT(%s%s) " on table " T4LIT(%s,table) " failed in condition " T4LIT(%s,warning) "\n",
#[                   on ? "" : "not ", get_reqcond_txt(reqcond), parts[i], cond);
#[             result = false;
#[             goto free_mem; // breaking out of outermost loop
#[         } else {
#[             result = true;
#[             goto free_mem; // condition holds, breaking out of outermost loop
#}         }

#[     }

#[ debug("    " T4LIT(!,warning) " No table given in condition " T4LIT(%s,warning) "\n", cond);
#[ result = false;

#[ free_mem:
#[     free(parts[0]);
#[     return result;
#} }
#[

#{ bool check_controlflow_requirements(fake_cmd_t cmd) {
#[     bool result = true;
#[     const char* reqs = cmd.requirements[0];
#[     char* conds[MAX_PART_COUNT];
#[     int part_count = split(conds, reqs, ",");
#[     for (int i = 0; i < part_count; ++i) {
#[         result &= check_cond(conds[i]);
#[     }
#[     free(conds[0]);
#[     return result;
#} }
#[

#} #endif
