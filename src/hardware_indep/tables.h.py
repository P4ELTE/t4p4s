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


#{ enum table_names {
for table in hlir.tables:
    #[ TABLE_${table.name},
#[ TABLE_,
#} };


#[ void exact_add_promote(int tableid, uint8_t* key, uint8_t* value, bool is_const_entry, bool should_print);
#[ void lpm_add_promote(int tableid, uint8_t* key, uint8_t depth, uint8_t* value, bool is_const_entry, bool should_print);
#[ void ternary_add_promote(int tableid, uint8_t* key, uint8_t* mask, uint8_t* value, bool is_const_entry, bool should_print);
#[ void table_setdefault_promote(int tableid, uint8_t* value);
