# Copyright 2016 Eotvos Lorand University, Budapest, Hungary
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
from utils.misc import addError, addWarning

#[ #include "dataplane.h"
#[ #include "actions.h"
#[ #include "tables.h"
#[ #include "stateful_memory.h"
#[
#[ extern struct all_metadatas_t all_metadatas;

#[ lookup_table_t table_config[NB_TABLES] = {
for table in hlir16.tables:
    tmt = table.match_type if hasattr(table, 'key') else "none"
    ks  = table.key_length_bytes if hasattr(table, 'key') else 0
    if ks==0 and table.key.keyElements[0].header_name=='meta': # TODO: check if it is consistent...
        ks = "sizeof(all_metadatas.metafield_%s)" % table.key.keyElements[0].field_name
        
    #[ {
    #[  .name= "${table.name}",
    #[  .id = TABLE_${table.name},
    #[  .type = LOOKUP_$tmt,

    #[  .entry = {
    #[      .entry_count = 0,

    #[      .key_size = $ks,

    #[      .entry_size = sizeof(struct ${table.name}_action) + sizeof(local_state_${table.name}_t) + sizeof(entry_validity_t),
    #[      .action_size   = sizeof(struct ${table.name}_action),
    #[      .state_size    = sizeof(local_state_${table.name}_t),
    #[      .validity_size = sizeof(entry_validity_t),
    #[  },

    #[  .min_size = 0,
    #[  .max_size = 250000,
    #[ },
#[ };

