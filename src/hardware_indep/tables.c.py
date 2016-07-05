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
from utils.hlir import getTypeAndLength
import p4_hlir.hlir.p4_stateful as p4_stateful

#[ #include "dataplane.h"
#[ #include "actions.h"
#[ #include "data_plane_data.h"
#[
#[ lookup_table_t table_config[NB_TABLES] = {
for table in hlir.p4_tables.values():
    table_type, key_length = getTypeAndLength(table)
    #[ {
    #[  .name= "${table.name}",
    #[  .type = ${table_type},
    #[  .key_size = ${key_length},
    #[  .val_size = sizeof(struct ${table.name}_action),
    #[  .min_size = 0, //${table.min_size},
    #[  .max_size = 255 //${table.max_size}
    #[ },
#[ };

#[ counter_t counter_config[NB_COUNTERS] = {
for counter in hlir.p4_counters.values():
    #[ {
    #[  .name= "${counter.name}",
    if counter.instance_count is not None:
        #[ .size = ${counter.instance_count},
    elif counter.binding is not None:
        btype, table = counter.binding
        if btype is p4_stateful.P4_DIRECT:
            #[ .size = ${table.max_size},
    else:
        #[ .size = 1,
    #[  .min_width = ${32 if counter.min_width is None else counter.min_width},
    #[  .saturating = ${1 if counter.saturating else 0}
    #[ },
#[ };
