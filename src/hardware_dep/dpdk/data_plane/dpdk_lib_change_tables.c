// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

// This file is included directly from `dpdk_lib.c`.

#include "tables.h"
#include "dataplane_impl.h"

void change_replica(int socketid, int tid, int replica) {
    for (unsigned lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
        if (rte_lcore_is_enabled(lcore_id) == 0) continue;

        int core_socketid = rte_lcore_to_socket_id(lcore_id);
        if (core_socketid != socketid) continue;

        struct lcore_conf* qconf = &lcore_conf[lcore_id];
        qconf->state.tables[tid] = state[socketid].tables[tid][replica]; // TODO should this be atomic?
        state[socketid].active_replica[tid] = replica;
    }
}

#define CHANGE_TABLE(fun, par...) \
{ \
    { \
        int current_replica = state[socketid].active_replica[tableid]; \
        int next_replica = (current_replica+1)%NB_REPLICA; \
        fun(state[socketid].tables[tableid][next_replica], par); \
        change_replica(socketid, tableid, next_replica); \
        usleep(TABCHANGE_SLEEP_MICROS); \
        for (int current_replica = 0; current_replica < NB_REPLICA; current_replica++) { \
            if (current_replica != next_replica) { \
                fun(state[socketid].tables[tableid][current_replica], par); \
            } \
        } \
    } \
}

#if defined T4P4S_DEBUG && T4P4S_DEBUG >= 1
    #define T4P4S_FORCE_PRINT true
#else
    #define T4P4S_FORCE_PRINT false
#endif

#ifdef T4P4S_DEBUG
#define FORALL_PRINTOUT(txt1, txt2, b, is_const_entry, should_print) \
    if (!is_const_entry)    ++state[socketid].tables[tableid][0]->init_entry_count; \
    if (should_print || T4P4S_FORCE_PRINT) { \
        lookup_table_entry_info_t entry_info = state[socketid].tables[tableid][0]->entry; \
        lookup_table_t tcfg = table_config[tableid]; \
        char params_str[1024]; \
        show_params_by_action_id(params_str, tcfg.id, entry->id, &(entry->params)); \
        dbg_bytes(key, entry_info.key_size, " " T4LIT(ctl>,incoming) " " txt1 " " T4LIT(%s,table) txt2 ": " T4LIT(%s,action) "%s <- ", \
                  tcfg.short_name, action_short_names[entry->id], params_str); \
    }
#else
#define FORALL_PRINTOUT(txt1, txt2, b, is_const_entry, should_print)
#endif

#define FORALLNUMANODES(txt1, txt2, b, is_const_entry, should_print) \
    for (int socketid = 0; socketid < NB_SOCKETS; socketid++) \
        if (state[socketid].tables[0][0] != NULL) { \
            FORALL_PRINTOUT(txt1, txt2, b, is_const_entry, should_print) \
            b \
        }

#define NO_ACTION_NAME ".NoAction"

#define FORALLNUMANODES_NOKEY(txt1, b, show_info) \
    for (int socketid = 0; socketid < NB_SOCKETS; socketid++) \
        if (state[socketid].tables[0][0] != NULL) { \
            if (socketid == 0 && !table_config[tableid].is_hidden) { \
                if (show_info && strcmp(action_short_names[entry->id], NO_ACTION_NAME)) { \
                    debug("    : " txt1 " " T4LIT(%s,table) ": " T4LIT(%s,action) "\n", table_config[tableid].short_name, action_short_names[entry->id]); \
                } \
            } \
            b \
        }

void exact_add_promote(table_name_e tableid, uint8_t* key, ENTRYBASE* entry, bool is_const_entry, bool should_print) {
    FORALLNUMANODES("Add", "/" T4LIT(exact), CHANGE_TABLE(exact_add, key, entry), is_const_entry, should_print)
}
void lpm_add_promote(table_name_e tableid, uint8_t* key, uint8_t depth, ENTRYBASE* entry, bool is_const_entry, bool should_print) {
    FORALLNUMANODES("Add", "/" T4LIT(LPM), CHANGE_TABLE(lpm_add, key, depth, entry), is_const_entry, should_print)
}
void ternary_add_promote(table_name_e tableid, uint8_t* key, uint8_t* mask, ENTRYBASE* entry, bool is_const_entry, bool should_print) {
    FORALLNUMANODES("Add", "/" T4LIT(ternary), CHANGE_TABLE(ternary_add, key, mask, entry), is_const_entry, should_print)
}
void table_setdefault_promote(table_name_e tableid, ENTRYBASE* entry, bool show_info) {
    FORALLNUMANODES_NOKEY("Set default action on", CHANGE_TABLE(table_set_default_action, entry), show_info)
}
