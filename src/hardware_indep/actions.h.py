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

#[ #define FIELD(name, length) uint8_t name[(length + 7) / 8];

def unique_stable(items):
    """Returns only the first occurrence of the items in a list.
    Equivalent to unique_everseen from Python 3."""
    from collections import OrderedDict
    return list(OrderedDict.fromkeys(items))

# NOTE: hlir16 version
#[ enum TODO16_actions {
a = {}
for table in hlir16.tables:
    for action in unique_stable(table.actions):
        #[ action_${action.action_object.name},
#[ };

#[ enum actions {
a = []
for table in hlir.p4_tables.values():
    for action in table.actions:
        if a.count(action.name) == 0:
            a.append(action.name)
            #[ action_${action.name},
#[ };

# NOTE: hlir16 version
for table in hlir16.tables:
    for action in table.actions:
        # if len(action.action_object.parameters) == 0:
        #     continue

        #[ struct TODO16_action_${action.action_object.name}_params {
        for param in action.action_object.parameters:
            # // TODO possibly needs fix...
            # #[ FIELD(${param.name}, ${param.length});
            pass
        #[ };

for table in hlir.p4_tables.values():
    for action in table.actions:
        if not action.signature:
            continue

        #[ struct action_${action.name}_params {
        for name, length in zip(action.signature, action.signature_widths):
            #[ FIELD(${name}, ${length});
        #[ };

# NOTE: hlir16 version
for table in hlir16.tables:
    #[ struct TODO16_${table.name}_action {
    #[     int action_id;
    #[     union {
    for action in table.actions:
        # TODO what if the action is not a method call?
        # TODO what if there are more actions?
        action_method_name = action.expression.method.name
        #[         struct TODO16_action_${action.action_object.name}_params ${action_method_name}_params;
    #[     };
    #[ };

for table in hlir.p4_tables.values():
    #[ struct ${table.name}_action {
    #[     int action_id;
    #[     union {
    for action in table.actions:
        if action.signature:
            #[ struct action_${action.name}_params ${action.name}_params;
    #[     };
    #[ };



# NOTE: hlir16 version
for table in hlir16.tables:
    #[ void TODO16_apply_table_${table.name}(packet_descriptor_t *pd, lookup_table_t** tables);
    for action in table.actions:
        action_method_name = action.expression.method.name
        #[ void TODO16_action_code_${action.action_object.name}(packet_descriptor_t *pd, lookup_table_t **tables, struct TODO16_action_${action_method_name}_params);


for table in hlir.p4_tables.values():
    #[ void apply_table_${table.name}(packet_descriptor_t *pd, lookup_table_t** tables);
    for action in table.actions:
        if action.signature:
            #[ void action_code_${action.name}(packet_descriptor_t *pd, lookup_table_t **tables, struct action_${action.name}_params);
        else:
            #[ void action_code_${action.name}(packet_descriptor_t *pd, lookup_table_t **tables);

#[ #endif
