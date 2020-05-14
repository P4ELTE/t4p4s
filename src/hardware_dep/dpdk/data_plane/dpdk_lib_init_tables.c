// Copyright 2018 Eotvos Lorand University, Budapest, Hungary
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


// This file is included directly from `dpdk_lib.c`.


extern void create_table(lookup_table_t* t, int socketid);
extern void flush_table(lookup_table_t* t);

void create_tables_on_socket(int socketid)
{
    for (int i = 0; i < NB_TABLES; i++) {
        lookup_table_t t = table_config[i];

        for (int j = 0; j < NB_REPLICA; j++) {
            state[socketid].tables[i][j] = malloc(sizeof(lookup_table_t));
            memcpy(state[socketid].tables[i][j], &t, sizeof(lookup_table_t));
            state[socketid].tables[i][j]->instance = j;
            create_table(state[socketid].tables[i][j], socketid);
            #ifdef T4P4S_DEBUG
                state[socketid].tables[i][j]->init_entry_count = 0;
            #endif
        }

        state[socketid].active_replica[i] = 0;
    }
}

void create_table_on_lcore(unsigned lcore_id)
{
    if (rte_lcore_is_enabled(lcore_id) == 0) return;

    int socketid = get_socketid(lcore_id);

    if (state[socketid].tables[0][0] == NULL) {
        create_tables_on_socket(socketid);
    }

    // TODO is it necessary to store the table in two places?
    for (int i = 0; i < NB_TABLES; i++) {
        struct lcore_conf* qconf = &lcore_conf[lcore_id];
        qconf->state.tables[i] = state[socketid].tables[i][0];
    }
}

void init_tables()
{
#ifdef T4P4S_DEBUG
    char table_names[1024];
    char* nameptr = table_names;
    nameptr += sprintf(nameptr, " :::: Init tables on all cores (" T4LIT(%d) " replicas each): ", NB_REPLICA);
    for (int i = 0; i < NB_TABLES; i++) {
        lookup_table_t t = table_config[i];
        nameptr += sprintf(nameptr, "%s" T4LIT(%s,table), i == 0 ? "" : ", ", t.name);
    }
    debug("%s\n", table_names);
#endif

    for (unsigned lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
        create_table_on_lcore(lcore_id);
    }
}

void flush_tables_on_socket(int socketid)
{
    for (int i = 0; i < NB_TABLES; i++) {
        lookup_table_t t = table_config[i];

        for (int j = 0; j < NB_REPLICA; j++) {
            flush_table(state[socketid].tables[i][j]);
        }

        state[socketid].active_replica[i] = 0;
    }
}

void flush_table_on_lcore(unsigned lcore_id)
{
    if (rte_lcore_is_enabled(lcore_id) == 0) return;

    int socketid = get_socketid(lcore_id);
    if (state[socketid].tables[0][0] == NULL) return;
    flush_tables_on_socket(socketid);
}

void flush_tables()
{
    debug("Flushing tables on all cores\n");
    for (unsigned lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
        flush_table_on_lcore(lcore_id);
    }
}
