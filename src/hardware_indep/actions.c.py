# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from utils.misc import addError, addWarning 
from utils.codegen import format_declaration, format_statement, format_expr, format_type, get_method_call_env
from compiler_common import types, unique_everseen

#[ #include "dpdk_lib.h"
#[ #include "actions.h"
#[ #include <unistd.h>
#[ #include "util.h"
#[ #include "util_packet.h"

#[ extern ctrl_plane_backend bg;


################################################################################

# Forward declarations for externs
for method in hlir.methods:
    calls = [node for node in hlir.all_nodes.by_type('MethodCallExpression') if 'path' in node.method and node.method.path.name == method.name]
    for mcall in calls:
        with types(get_method_call_env(mcall, method.name)):
            t = method.type
            ret_type = format_type(t.returnType)
            args = ", ".join([format_expr(arg) for arg in t.parameters.parameters] + ['SHORT_STDPARAMS'])

            #[ extern ${ret_type} ${method.name}(${args});

################################################################################


#{ char* action_names[] = {
for table in hlir.tables:
    for action in unique_everseen(table.actions):
        #[     "${action.action_object.name}",
#} };


for ctl in hlir.controls:
    for act in ctl.actions:
        fun_params = ["SHORT_STDPARAMS", f"action_{act.name}_params_t parameters"]

        #{ void action_code_${act.name}(${', '.join(fun_params)}) {
        #[     uint32_t value32, res32, mask32;
        #[     (void)value32; (void)res32; (void)mask32;
        #[     control_locals_${ctl.name}_t* local_vars = (control_locals_${ctl.name}_t*) pd->control_locals;

        for stmt in act.body.components:
            global pre_statement_buffer
            global post_statement_buffer
            pre_statement_buffer = ""
            post_statement_buffer = ""

            code = format_statement(stmt, ctl)
            if pre_statement_buffer != "":
                #= pre_statement_buffer
                pre_statement_buffer = ""
            #= code
            if post_statement_buffer != "":
                #= post_statement_buffer
                post_statement_buffer = ""
        #} }
        #[
