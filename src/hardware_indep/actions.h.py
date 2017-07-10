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
#[ #ifndef __ACTION_INFO_GENERATED_H__
#[ #define __ACTION_INFO_GENERATED_H__
#[ 
#[
#[ #define FIELD(name, length) uint8_t name[(length + 7) / 8];
#[

#[ enum actions {
a = []
for table in hlir.p4_tables.values():
    for action in table.actions:
        if a.count(action.name) == 0:
            a.append(action.name)
            #[ action_${action.name},
#[ };

for table in hlir.p4_tables.values():
    for action in table.actions:
        if action.signature:
            #[ struct action_${action.name}_params {
            for name, length in zip(action.signature, action.signature_widths):
                #[ FIELD(${name}, ${length});
            #[ };

# TODO the following part will probably look similar to this
# TODO but changing the actions' names have to be done everywhere at the same time
# TODO because the names don't stay the same
# for table in hlir16.tables:
#     #[ struct ${table.name}_action {
#     #[     int action_id;
#     #[     union {
#     for action in table.actions.actionList:
#         # TODO what if the action is not a method call?
#         # TODO what if there are more actions?
#         action_name = action.expression.method.path.name
#         #[         struct action_${action_name}_params ${action_name}_params;
#     #[     };
#     #[ };

for table in hlir.p4_tables.values():
    #[ struct ${table.name}_action {
    #[     int action_id;
    #[     union {
    for action in table.actions:
        if action.signature:
            #[ struct action_${action.name}_params ${action.name}_params;
    #[     };
    #[ };

for table in hlir.p4_tables.values():
    #[ void apply_table_${table.name}(packet_descriptor_t *pd, lookup_table_t** tables);
    for action in table.actions:
        if action.signature:
            #[ void action_code_${action.name}(packet_descriptor_t *pd, lookup_table_t **tables, struct action_${action.name}_params);
        else:
            #[ void action_code_${action.name}(packet_descriptor_t *pd, lookup_table_t **tables);

#[ #endif
