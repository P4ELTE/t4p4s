# SPDX-License-Identifier: Apache-2.0
# Copyright 2018 Eotvos Lorand University, Budapest, Hungary

from compiler_log_warnings_errors import addError
from utils.codegen import format_expr, format_type, format_statement, format_declaration
from compiler_common import unique_everseen

def gen_make_smem_code(smem, table = None):
    size = smem.size
    type = smem.urtype.name

    # TODO have just one lock even if there are two components
    for c in smem.components:
        cname =  c['name']
        ctype =  c['type']
        if smem.smem_type == "register":
            signed = "int" if smem.is_signed else "uint"
            size = 8 if smem.size <= 8 else 16 if smem.size <= 16 else 32 if smem.size <= 32 else 64
            c['vartype'] = f"register_{signed}{size}_t"
            #[     lock_t lock_${smem.smem_type}_$cname[${smem.amount}];
            #[     ${c['vartype']} ${smem.smem_type}_$cname[${smem.amount}];
            #[     int ${smem.smem_type}_${cname}_amount;
        elif table:
            sname = f"{cname}_{table.name}"

            #[     lock_t lock_$sname;
            #[     ${type}_t $sname;
            #[
        else:
            #[     lock_t lock_$cname[${smem.amount}];
            #[     ${type}_t $cname[${smem.amount}];
            #[     int ${cname}_amount;

        #[


#[ #pragma once

#[ #include "common.h"
#[ #include "aliases.h"
#[ #include "dpdk_smem.h"
#[ #include "gen_include.h"

#{ typedef struct {
for table, smem in hlir.all_meters + hlir.all_counters:
    if not smem.is_direct:
        continue
    #= gen_make_smem_code(smem, table)
for smem in unique_everseen((smem for table, smem in hlir.all_meters + hlir.all_counters if not smem.is_direct)):
    #= gen_make_smem_code(smem)
for smem in hlir.registers:
    #= gen_make_smem_code(smem)



# temp = {action: action.flatmap('parameters.parameters') for action in hlir.tables.flatmap('control.controlLocals').filter('node_type', 'P4Action')}

local_params = unique_everseen(hlir.tables.flatmap('control.controlLocals').filter('node_type', 'P4Action').flatmap('parameters.parameters'))
all_locals = unique_everseen((param.name, format_type(param.type)) for param in local_params)
all_locals_dict = dict(all_locals)
if len(all_locals) != len(all_locals_dict):
    names = [name for name, _type in all_locals]
    dups = unique_everseen(name for name in names if names.count(name) > 1)
    for dup in dups:
        dup_types = {f'{format_type(lp.type)}': f'{lp.type.node_type}[{lp.type.size}]' for lp in local_params if lp.name == dup}
        description = ", ".join(f'{t} aka {dup_types[t]}' for t in dup_types)
        addError("Collecting local variables of controls", f"Local variable {dup} is used with different types, which is currently unsupported: {description}")

for locname, loctype in all_locals:
    #[     $loctype $locname;

# Note: currently all control locals are put together into the global state
for ctl in hlir.controls:
    for local_var_decl in (ctl.controlLocals['Declaration_Variable'] + ctl.controlLocals['Declaration_Instance']).filterfalse('urtype.node_type', 'Type_Header').filterfalse(lambda n: 'smem_type' in n):
        #[     ${format_type(local_var_decl.urtype, varname = local_var_decl.name, resolve_names = False)};




#} } global_state_t;

#[ extern global_state_t global_smem;


#[ static lock_t ingress_lock;
#[ static lock_t egress_lock;
