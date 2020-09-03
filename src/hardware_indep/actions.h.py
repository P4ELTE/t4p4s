# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_type
from compiler_common import unique_everseen

#[ #ifndef __ACTIONS_H__
#[ #define __ACTIONS_H__

#[ #include "dataplane.h"
#[ #include "common.h"

# Note: this is for Digest_t
#[ #include "ctrl_plane_backend.h"

# TODO this should not be here in the indep section
#[ #include "dpdk_smem.h"

#[ #define FIELD(name, length) uint8_t name[(length + 7) / 8];


#{ enum actions {
for table in hlir.tables:
    for action in unique_everseen(table.actions):
        #[     action_${action.action_object.name},
#[      action_,
#} };

for ctl in hlir.controls:
    for act in ctl.actions:
        #{ typedef struct action_${act.name}_params_s {
        for param in act.parameters.parameters:
            paramtype = param.urtype
            #[     FIELD(${param.name}, ${paramtype.size});

        if len(act.parameters.parameters) == 0:
            #[     FIELD(DUMMY_FIELD, 0);
        #} } action_${act.name}_params_t;

for struct in hlir.news.misc:
    #{ typedef struct ${struct.name}_s {
    for fld in struct.fields:
        #[     FIELD(${fld.name}, ${fld.urtype.size});

    if len(hlir.news.misc) == 0:
        #[     FIELD(DUMMY_FIELD, 0);
    #} } ${struct.name}_t;

for table in hlir.tables:
    #{ struct ${table.name}_action {
    #[     int action_id;
    #{     union {
    for action in table.actions:
        # TODO what if the action is not a method call?
        # TODO what if there are more actions?
        import inspect; import pprint; pprint.PrettyPrinter(indent=4,width=999,compact=True).pprint([f"actions.h.py@{inspect.getframeinfo(inspect.currentframe()).lineno}", action])
        import inspect; import pprint; pprint.PrettyPrinter(indent=4,width=999,compact=True).pprint([f"actions.h.py@{inspect.getframeinfo(inspect.currentframe()).lineno}", action.expression.method])
        action_method_name = action.expression.method.path.name
        #[         action_${action.action_object.name}_params_t ${action_method_name}_params;
    #}     };
    #} };



for table in hlir.tables:
    #[ void apply_table_${table.name}(packet_descriptor_t *pd, lookup_table_t** tables);
    for action in table.actions:
        aname = action.action_object.name
        import inspect; import pprint; pprint.PrettyPrinter(indent=4,width=999,compact=True).pprint([f"actions.h.py@{inspect.getframeinfo(inspect.currentframe()).lineno}", action])
        import inspect; import pprint; pprint.PrettyPrinter(indent=4,width=999,compact=True).pprint([f"actions.h.py@{inspect.getframeinfo(inspect.currentframe()).lineno}", action.expression.method])
        mname = action.expression.method.path.name

        #[ void action_code_$aname(packet_descriptor_t *pd, lookup_table_t **tables, action_${mname}_params_t);


for ctl in hlir.controls:
    #{ typedef struct control_locals_${ctl.name}_s {
    for local_var_decl in ctl.controlLocals['Declaration_Variable'] + ctl.controlLocals['Declaration_Instance']:
        if 'type_ref' in local_var_decl.type:
            if local_var_decl.urtype.name in ['counter', 'direct_counter', 'meter']:
                continue
        else:
            if local_var_decl.type('baseurtype.name', lambda n: n in ['direct_meter', 'register']):
                continue

        postfix = "_t" if local_var_decl.type.node_type == 'Type_Name' else ""
        #[ ${format_type(local_var_decl.type, resolve_names = False)}$postfix ${local_var_decl.name};

    # TODO is there a more appropriate way to store registers?
    for reg in hlir.registers:
        #[ ${format_type(reg.type, resolve_names = False)} ${reg.name};

    #} } control_locals_${ctl.name}_t;

#[ #endif
