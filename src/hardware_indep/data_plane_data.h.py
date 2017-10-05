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
#[ #define NB_TABLES ${len(hlir16.tables)}
#[
#[ enum table_names {
for table in hlir16.tables:
    #[ TABLE_${table.name},
#[ TABLE_
#[ };
#[
#[ // TODO feature temporarily not supported (hlir16)
#[ #define NB_COUNTERS 0
#[
#[ enum counter_names {
#[ COUNTER_
#[ };
#[
#[ // TODO feature temporarily not supported (hlir16)
#[ #define NB_REGISTERS 0
#[
#[ enum register_names {
#[ REGISTER_
#[ };
#[
#[ struct uint8_buffer_t {
#[     int      buffer_size;
#[     uint8_t* buffer;
#[ };
#[
#[ #endif // __DATA_PLANE_DATA_H__
