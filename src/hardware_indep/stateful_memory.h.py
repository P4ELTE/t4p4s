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



all_locals = unique_everseen([(param.name, format_type(param.type)) for table in hlir.tables for local in table.control.controlLocals["P4Action"] for param in local.parameters.parameters])
all_locals_dict = dict(all_locals)
if len(all_locals) != len(all_locals_dict):
    names = [name for name, _type in all_locals]
    dups = unique_everseen([name for name in names if names.count(name) > 1])
    addError("Collecting counters, meters, registers and controls' local variables", "The following names are used with different types, which is currently unsupported: {}".format(", ".join(dups)))

for locname, loctype in all_locals:
    #[     $loctype $locname;


smems = ('counter', 'direct_counter', 'meter', 'direct_meter', 'register')

# Note: currently all control locals are put together into the global state
for ctl in hlir.controls:
    for local_var_decl in (ctl.controlLocals['Declaration_Variable'] + ctl.controlLocals['Declaration_Instance']).filterfalse('urtype.node_type', 'Type_Header').filterfalse('urtype.name', smems):
        #[     ${format_type(local_var_decl.type, varname = local_var_decl.name, resolve_names = False)};




#} } global_state_t;

#[ extern global_state_t global_smem;


#[ static lock_t ingress_lock;
#[ static lock_t egress_lock;
