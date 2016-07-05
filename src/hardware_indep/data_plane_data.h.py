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
#[ #ifndef __DATA_PLANE_DATA_H__
#[ #define __DATA_PLANE_DATA_H__
#[
#[ #include "parser.h"
#[
#[ #define NB_TABLES ${len(hlir.p4_tables)}
#[
#[ enum table_names {
for table in hlir.p4_tables.values():
    #[ TABLE_${table.name},
#[ TABLE_
#[ };
#[
#[ #define NB_COUNTERS ${len(hlir.p4_counters)}
#[
#[ enum counter_names {
for counter in hlir.p4_counters.values():
    #[ COUNTER_${counter.name},
#[ COUNTER_
#[ };
#[ 
#[ #endif // __DATA_PLANE_DATA_H__
