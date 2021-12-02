# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_type
from utils.extern import get_smem_name
from compiler_common import unique_everseen

#[ #pragma once

#[ #include "dataplane.h"
#[ #include "common.h"
#[ #include "gen_include.h"

#[ #include "util_packet.h"
#[ #include "stateful_memory_type.h"

# Note: this is for Digest_t
#[ #include "ctrl_plane_backend.h"

# TODO this should not be here in the indep section
#[ #include "dpdk_smem.h"

#[ #define FIELD(name, length) uint8_t name[(length + 7) / 8];


#{ typedef enum {
for table in hlir.tables:
    for action in unique_everseen(table.actions):
        #[     action_${action.action_object.name},
    if len(table.actions) == 0:
        #[     action_,
if len(hlir.tables) == 0:
    #[     action_DUMMY_ACTION_0,
#} } actions_e;

for ctl in hlir.controls:
    for act in ctl.actions:
        #{ typedef struct {
        for param in act.parameters.parameters:
            paramtype = param.urtype
            #[     ${format_type(param.urtype, varname = param.name)};

        if len(act.parameters.parameters) == 0:
            #[     FIELD(DUMMY_FIELD, 0);
        #} } action_${act.name}_params_t;
        #[


for table in hlir.tables:
    #{ typedef union {
    for action in table.actions:
        action_method_name = action.expression.method.path.name
        #[     action_${action.action_object.name}_params_t ${action_method_name}_params;
    #} } ${table.name}_action_params_t;
    #[

for table in hlir.tables:
    #{ typedef struct {
    #[     actions_e                     id;
    #[     ${table.name}_action_params_t params;
    #} } ${table.name}_action_t;
    #[



for table in hlir.tables:
    #[ void apply_table_${table.name}(SHORT_STDPARAMS);
    for action in table.actions:
        aname = action.action_object.name
        mname = action.expression.method.path.name

        #[ void action_code_$aname(action_${mname}_params_t, SHORT_STDPARAMS);

for ctl in hlir.controls:
    #{ typedef struct {
    for local_var_decl in ctl.local_var_decls.filterfalse('urtype.node_type', 'Type_Header'):
        if 'smem_type' in (smem := local_var_decl):
            if (reg := smem).smem_type == 'register':
                varnames = [get_smem_name(reg)]
            else:
                varnames = [get_smem_name(inst) for inst in smem.insts]
        elif (extern := local_var_decl.urtype).node_type == 'Type_Extern':
            if extern.repr is None:
                continue
            
            varnames = [f'EXTERNNAME({local_var_decl.name})']
        else:
            varnames = [local_var_decl.name]

        targs = local_var_decl.type('arguments', [])
        for varname in varnames:
            #[     ${format_type(local_var_decl.urtype, varname = varname, resolve_names = False, type_args = targs)};

    #} } control_locals_${ctl.name}_t;
    #[
