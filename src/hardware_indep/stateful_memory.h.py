# SPDX-License-Identifier: Apache-2.0
# Copyright 2018 Eotvos Lorand University, Budapest, Hungary

from compiler_log_warnings_errors import addError
from utils.codegen import format_expr, format_type, format_statement, format_declaration
from utils.extern import get_smem_name
from compiler_common import unique_everseen


#[ #pragma once

#[ #include "common.h"
#[ #include "aliases.h"
#[ #include "dpdk_smem.h"
#[ #include "gen_include.h"
#[ #include "stateful_memory_type.h"

nonregs = unique_everseen(smem for _, smem in hlir.smem.all_counters + hlir.smem.all_meters)

if len(hlir.smem.all_counters) + len(hlir.smem.all_meters) + len(hlir.smem.registers) > 0:
    #{ typedef enum {
    for smem in nonregs:
        amount = 1 if smem.is_direct else smem.amount
        #[     ${get_smem_name(smem, ['amount'])} = $amount,
    for inst in hlir.smem.registers:
        #[     ${get_smem_name(inst, ['amount'])} = ${inst.amount},
    #} } global_smem_amounts_e;
    #[

#{ typedef struct {
for smem in nonregs:
    #[     SMEMTYPE(${smem.smem_type}) ${get_smem_name(smem)}[${get_smem_name(smem, ['amount'])}];
for inst in hlir.smem.registers:
    signed = "int" if inst.is_signed else "uint"
    size = 8 if inst.size <= 8 else 16 if inst.size <= 16 else 32 if inst.size <= 32 else 64
    #[     REGTYPE($signed,$size) ${get_smem_name(inst)}[${get_smem_name(inst, ['amount'])}];
#[

for smem in nonregs:
    amount = 1 if smem.is_direct else smem.amount
    #[     lock_t ${get_smem_name(smem, ['lock'])}[${amount}];
#[


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
#[

# Note: currently all control locals are put together into the global state
for ctl in hlir.controls:
    for local_var_decl in (ctl.controlLocals['Declaration_Variable'] + ctl.controlLocals['Declaration_Instance']).filterfalse('urtype.node_type', 'Type_Header').filterfalse(lambda n: 'smem_type' in n):
        if (extern := local_var_decl.urtype).node_type == 'Type_Extern' and extern.repr is None:
            #[     // the extern ${extern.name} has no representation
            continue
        extern_name = f'EXTERNNAME({local_var_decl.name})'
        #[     ${format_type(local_var_decl.urtype, varname = extern_name, resolve_names = False)};
#} } global_state_t;

#[ extern global_state_t global_smem;


#[ static lock_t ingress_lock;
#[ static lock_t egress_lock;
