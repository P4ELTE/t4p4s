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

from utils.codegen import format_expr, format_statement, statement_buffer_value, format_declaration


# In v1model, all software memory cells are represented as 32 bit integers
def smem_repr_type(smem, smem_type):
    return "rte_atomic32_t"


def smem_components(smem, smem_type):
    base_type = smem_repr_type(smem, smem_type)

    if smem_type == 'reg':
        return [{"type": base_type, "name": smem.name}]

    # TODO set these in hlir16_attrs
    smem.packets_or_bytes = smem.arguments['Member'][0].member
    smem.smem_for = {
        "packets": smem.packets_or_bytes in ("packets", "packets_and_bytes"),
        "bytes":   smem.packets_or_bytes in (  "bytes", "packets_and_bytes"),
    }

    pkts_name  = "{}_{}_packets".format(smem_type, smem.name)
    bytes_name = "{}_{}_bytes".format(smem_type, smem.name)

    pbs = {
        "packets":           [{"for": "packets", "type": base_type, "name": pkts_name}],
        "bytes":             [{"for":   "bytes", "type": base_type, "name": bytes_name}],

        "packets_and_bytes": [{"for": "packets", "type": base_type, "name": pkts_name},
                              {"for":   "bytes", "type": base_type, "name": bytes_name}],
    }

    return pbs[smem.packets_or_bytes]


def gen_make_smem_code(smem, smem_size, smem_type, locked = False):
    # TODO set these in hlir16_attrs
    smem.smem_type  = smem_type
    smem.components = smem_components(smem, smem_type)


    for c in smem.components:
        if locked:
            #[ lock_t         lock_${c['name']}[$smem_size];
        #[ ${c['type']} ${c['name']}[$smem_size];


#[ #ifndef __STATEFUL_MEMORY_H_
#[ #define __STATEFUL_MEMORY_H_

#[ #include "aliases.h"
#[ #include <rte_atomic.h>


#{ typedef struct global_state_s {
for t, meter in hlir16.meters:
    size = meter.arguments[0].value

    #= gen_make_smem_code(meter, size, 'direct_meter', True)

for t, counter in hlir16.counters:
    size = counter.arguments[0].value

    #= gen_make_smem_code(counter, size, 'direct_counter', True)

for t, reg in hlir16.registers:
    # result_type_bitsize = reg.type.arguments[0].size
    size = reg.arguments[0].value

    #= gen_make_smem_code(reg, size, 'reg', True)

#} } global_state_t;


for table in hlir16.tables:
    #{ typedef struct local_state_${table.name}_s {
    for counter in table.counters:
        #= gen_make_smem_code(counter, 1, 'counter')

    for meter in table.meters:
        #= gen_make_smem_code(meter, 1, 'meter')

    #} } local_state_${table.name}_t;

#[ #endif
