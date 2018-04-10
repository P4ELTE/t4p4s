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
#[ #include "data_plane_data.h"
#[

#[ lookup_table_t table_config[NB_TABLES] = {
for table in hlir16.tables:
    #[ {
    #[  .name= "${table.name}",
    #[  .id = TABLE_${table.name},
    #[  .type = LOOKUP_${table.match_type},
    #[  .key_size = ${table.key_length_bytes},
    #[  .val_size = sizeof(struct ${table.name}_action),
    #[  .min_size = 0,
    #[  .max_size = 250000
    #[ },
#[ };

#[ counter_t counter_config[NB_COUNTERS] = {
#[     // TODO feature temporarily not supported (hlir16)
#[ };

#[ p4_register_t register_config[NB_REGISTERS] = {
#[     // TODO feature temporarily not supported (hlir16)
#[ };
