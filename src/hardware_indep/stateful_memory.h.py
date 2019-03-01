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
def smem_repr_type(smem, smem_type, bit_width=32, is_signed=False):
    if is_signed:
        tname = "int"
    else:
        tname = "uint"

    for w in [8,16,32,64]:
        if bit_width <= w:
            return "reg_" + tname + str(w) + "_t"

    return "NOT_SUPPORTED"


def smem_components(smem, smem_type, bit_width=32, is_signed=False):
    base_type = smem_repr_type(smem, smem_type, bit_width, is_signed)

    if smem_type == 'reg':
        return [{"type": base_type, "name": smem.name}]

    member = [s.expression for s in smem.arguments if s.expression.node_type == 'Member'][0]

    # TODO set these in hlir16_attrs
    smem.packets_or_bytes = member.member
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


def gen_make_smem_code(smem, smem_size, smem_type, locked = False, bit_width = 32, is_signed = False):
    # TODO set these in hlir16_attrs
    smem.smem_type  = smem_type
    smem.components = smem_components(smem, smem_type, bit_width, is_signed)


    for c in smem.components:
        if locked:
            #[ lock_t         lock_${c['name']}[$smem_size];
        #[ ${c['type']} ${c['name']}[$smem_size];


#[ #ifndef __STATEFUL_MEMORY_H_
#[ #define __STATEFUL_MEMORY_H_

#[ #include "aliases.h"
#[ #include "dpdk_reg.h"

# // atomic vars

#for tbase in ["int","uint"]:
#    for bwidth in [8,16,32,64]:
#        #{ typedef struct reg_${tbase}${bwidth}_s {
#        #[    volatile ${tbase}${bwidth}_t value;
#        #} } reg_${tbase}${bwidth}_t;

#for tbase in ["int","uint"]:
#    for bwidth in [8,16,32,64]:
#        #[ void extern_register_write_${tbase}${bwidth}_t(reg_${tbase}${bwidth}_t* reg, uint32_t idx, ${tbase}${bwidth}_t value);
#        #[ void extern_register_read_${tbase}${bwidth}_t(reg_${tbase}${bwidth}_t* reg, uint32_t idx, ${tbase}${bwidth}_t* value);
#        #[ void init_register_${tbase}${bwidth}_t(reg_${tbase}${bwidth}_t* reg, uint32_t size);


#        #[    reg[idx].value = value;
#        #} }
#        #{ void extern_register_read_${tbase}${bwidth}_t(reg_${tbase}${bwidth}_t* reg, uint32_t idx, ${tbase}${bwidth}_t* value) {
#        #[    *value =  reg[idx].value;
#        #} }
#        #{ void init_register_${tbase}${bwidth}_t(reg_${tbase}${bwidth}_t* reg, uint32_t size)  {
#        #[    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
#        #} }



# #[ // end

#{ typedef struct global_state_s {
for t, meter in hlir16.meters:
    size = meter.arguments[0].expression.value

    #= gen_make_smem_code(meter, size, 'direct_meter', True)

for t, counter in hlir16.counters:
    size = counter.arguments[0].expression.value

    #= gen_make_smem_code(counter, size, 'direct_counter', True)

for t, reg in hlir16.registers:
    # result_type_bitsize = reg.type.arguments[0].size
    size = reg.arguments[0].expression.value
    # str(dir(reg))
    # str(reg.json_data)
    # str(reg.type.arguments[0])
    bit_width = reg.type.arguments[0].size
    is_signed = reg.type.arguments[0].isSigned
    # str(reg.type.arguments[0].isSigned)
    # str(reg.arguments[0].expression.value)

    #= gen_make_smem_code(reg, size, 'reg', True, bit_width, is_signed)

#} } global_state_t;

#[ static global_state_t global_smem;

for table in hlir16.tables:
    #{ typedef struct local_state_${table.name}_s {
    for counter in table.counters:
        #= gen_make_smem_code(counter, 1, 'counter')

    for meter in table.meters:
        #= gen_make_smem_code(meter, 1, 'meter')

    #} } local_state_${table.name}_t;

#[ static lock_t ingress_lock;
#[ static lock_t egress_lock;


#[ #endif
