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

from utils.misc import addError
from utils.codegen import format_expr, format_type, format_statement, statement_buffer_value, format_declaration


def unique_stable(items):
    """Returns only the first occurrence of the items in a list.
    Equivalent to unique_everseen from Python 3."""
    from collections import OrderedDict
    return list(OrderedDict.fromkeys(items))


def gen_make_smem_code(smem, table = None):
    size = smem.bit_width
    type = smem.type._baseType.path.name

    # TODO have just one lock even if there are two components
    for c in smem.components:
        cname =  c['name']
        ctype =  c['type']
        if smem.smem_type == "register":
            signed = "int" if smem.is_signed else "uint"
            bit_width = smem.bit_width
            size = 8 if bit_width <= 8 else 16 if bit_width <= 16 else 32 if bit_width <= 32 else 64
            c['vartype'] = "register_{}{}_t".format(signed, size)
            #[     lock_t lock_$cname[${smem.amount}];
            #[     ${c['vartype']} $cname[${smem.amount}];
            #[     int ${cname}_amount;
        elif table:
            sname = "{}_{}".format(cname, table.name)

            #[     lock_t lock_$sname;
            #[     ${smem.type._baseType.path.name}_t $sname;
            #[
        else:
            #[     lock_t lock_$cname[${smem.amount}];
            #[     ${smem.type._baseType.path.name}_t $cname[${smem.amount}];
            #[     int ${cname}_amount;

        #[


#[ #ifndef __STATEFUL_MEMORY_H_
#[ #define __STATEFUL_MEMORY_H_

#[ #include "common.h"
#[ #include "aliases.h"
#[ #include "dpdk_smem.h"

#{ typedef struct global_state_s {
for table, smem in hlir16.all_meters + hlir16.all_counters:
    if smem.smem_type not in ["direct_counter", "direct_meter"]:
        continue
    #= gen_make_smem_code(smem, table)
for smem in unique_stable([smem for table, smem in hlir16.all_meters + hlir16.all_counters if smem.smem_type not in ["direct_counter", "direct_meter"]]):
    #= gen_make_smem_code(smem)
for smem in hlir16.registers:
    #= gen_make_smem_code(smem)



all_locals = unique_stable([(param.name, format_type(param.type)) for table in hlir16.tables for local in table.control.controlLocals["P4Action"] for param in local.parameters.parameters])
all_locals_dict = dict(all_locals)
if len(all_locals) != len(all_locals_dict):
    names = [name for name, _type in all_locals]
    dups = unique_stable([name for name in names if names.count(name) > 1])
    addError("Collecting counters, meters, registers and controls' local variables", "The following names are used with different types, which is currently unsupported: {}".format(", ".join(dups)))

for locname, loctype in all_locals:
    #[     $loctype $locname;


# Note: currently all control locals are put together into the global state
for ctl in hlir16.controls:
    for local_var_decl in ctl.controlLocals['Declaration_Variable'] + ctl.controlLocals['Declaration_Instance']:
        if hasattr(local_var_decl.type, 'type_ref'):
            if local_var_decl.type.type_ref.name in ['counter', 'direct_counter', 'meter']:
                continue
        else:
            if local_var_decl.type('baseType.type_ref.name', lambda n: n in ['direct_meter', 'register']):
                continue
        postfix = "_t" if local_var_decl.type.node_type == 'Type_Name' else ""
        #[     ${format_type(local_var_decl.type, resolve_names = False)}$postfix ${local_var_decl.name};




#} } global_state_t;

#[ extern global_state_t global_smem;


#[ static lock_t ingress_lock;
#[ static lock_t egress_lock;


#[ #endif
