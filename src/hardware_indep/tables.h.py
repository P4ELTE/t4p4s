# SPDX-License-Identifier: Apache-2.0
# Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#[ #pragma once

#[ #include "stateful_memory.h"
#[ #include "actions.h"


#[ #define ENTRY(tname)    tname ## _entry_t
#[ #define ENTRYBASE       ENTRY(base_table)


# Note: a table entry contains a (possibly invalid) action and a state
#       the latter of which is not represented
#[ typedef base_table_action_t ENTRYBASE;
#[


for table in hlir.tables:
    #{ typedef struct {
    #[     actions_e                     id;
    #[     ${table.name}_action_params_t params;
    #} } ENTRY(${table.name});
    #[


#{ typedef enum {
for table in hlir.tables:
    #[     TABLE_${table.name},
#[ TABLE_,
#} } table_name_e;
#[


#[ void exact_add_promote  (table_name_e tableid, uint8_t* key,                ENTRYBASE* entry, bool is_const_entry, bool should_print);
#[ void lpm_add_promote    (table_name_e tableid, uint8_t* key, uint8_t depth, ENTRYBASE* entry, bool is_const_entry, bool should_print);
#[ void ternary_add_promote(table_name_e tableid, uint8_t* key, uint8_t* mask, ENTRYBASE* entry, bool is_const_entry, bool should_print);
#[ void table_setdefault_promote(table_name_e tableid, ENTRYBASE* entry, bool show_info);

#[ //=============================================================================

#[ // Computes the location of the validity field of the entry.
#[ bool* entry_validity_ptr(ENTRYBASE* entry, lookup_table_t* t);
