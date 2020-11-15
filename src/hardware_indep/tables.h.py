# SPDX-License-Identifier: Apache-2.0
# Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#[ #pragma once

#[ #include "stateful_memory.h"
#[ #include "actions.h"

#[ typedef bool entry_validity_t;

for table in hlir.tables:
    #{ typedef struct table_entry_${table.name}_s {
    #[     ${table.name}_action_t  action;
    #[     entry_validity_t        is_entry_valid;
    #} } table_entry_${table.name}_t;


#{ enum table_names {
for table in hlir.tables:
    #[ TABLE_${table.name},
#[ TABLE_,
#} };
