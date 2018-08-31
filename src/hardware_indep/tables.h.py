# Copyright 2018 Eotvos Lorand University, Budapest, Hungary
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#[ #ifndef __TABLES_H__
#[ #define __TABLES_H__

#[ #include "stateful_memory.h"
#[ #include "actions.h"

#[ typedef bool entry_validity_t;

for t in hlir16.tables:
    #{ typedef struct table_entry_${t.name}_s {
    #[     struct ${t.name}_action  action;
    #[     local_state_${t.name}_t  state;
    #[     entry_validity_t         is_entry_valid;
    #} } table_entry_${t.name}_t;


#[ #define NB_TABLES ${len(hlir16.tables)}

#{ enum table_names {
for table in hlir16.tables:
    #[ TABLE_${table.name},
#[ TABLE_,
#} };

#[ #endif
