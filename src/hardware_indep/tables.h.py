# SPDX-License-Identifier: Apache-2.0
# Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#[ #pragma once

#[ #include "stateful_memory.h"
#[ #include "actions.h"

#[ typedef bool entry_validity_t;

for table in hlir.tables:
    #{ typedef struct {
    #[     ${table.name}_action_t  action;
    #[     entry_validity_t        is_entry_valid;
    #} } table_entry_${table.name}_t;
    #[


#{ typedef enum {
for table in hlir.tables:
    #[     TABLE_${table.name},
#[ TABLE_,
#} } table_name_t;
#[


#[ void exact_add_promote  (table_name_t tableid, uint8_t* key,                uint8_t* value, bool is_const_entry, bool should_print);
#[ void lpm_add_promote    (table_name_t tableid, uint8_t* key, uint8_t depth, uint8_t* value, bool is_const_entry, bool should_print);
#[ void ternary_add_promote(table_name_t tableid, uint8_t* key, uint8_t* mask, uint8_t* value, bool is_const_entry, bool should_print);
#[ void table_setdefault_promote(table_name_t tableid, actions_t* value);

#[ //=============================================================================

#[ // Returns the action id stored in the table entry parameter.
#[ // Table entries have different types (${table.name}_action),
#[ // but all of them have to start with an int, the action id.
#[ int get_entry_action_id(const void* entry);

#[ // Returns the action id stored in the table entry parameter.
#[ // Table entries have different types (${table.name}_action),
#[ // but all of them have to start with an int, the action id.
#[ char* get_entry_action_name(const void* entry);

#[ // Computes the location of the validity field of the entry.
#[ bool* entry_validity_ptr(uint8_t* entry, lookup_table_t* t);
