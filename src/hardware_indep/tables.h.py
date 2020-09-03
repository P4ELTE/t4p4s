# SPDX-License-Identifier: Apache-2.0
# Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#[ #ifndef __TABLES_H__
#[ #define __TABLES_H__

#[ #include "stateful_memory.h"
#[ #include "actions.h"

#[ typedef bool entry_validity_t;

for t in hlir.tables:
    #{ typedef struct table_entry_${t.name}_s {
    #[     struct ${t.name}_action  action;
    #[     entry_validity_t         is_entry_valid;
    #} } table_entry_${t.name}_t;


#[ #define NB_TABLES ${len(hlir.tables)}

#{ enum table_names {
for table in hlir.tables:
    #[ TABLE_${table.name},
#[ TABLE_,
#} };

#[ #endif
