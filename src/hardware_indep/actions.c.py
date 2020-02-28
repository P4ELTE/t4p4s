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
from utils.misc import addError, addWarning 
from utils.codegen import format_declaration, format_statement, format_expr, format_type, type_env

#[ #include "dpdk_lib.h"
#[ #include "actions.h"
#[ #include <unistd.h>
#[ #include "util.h"
#[ #include "util_packet.h"

#[ extern ctrl_plane_backend bg;


# TODO remove the duplication (with dataplane.c.py)
class types:
    def __init__(self, new_type_env):
        global type_env
        self.env_vars = set()
        for v in new_type_env:
            if v in type_env:
                addWarning('adding a type environment', 'variable {} is already bound to type {}'.format(v, type_env[v]))
            else:
                self.env_vars.add(v)
                type_env[v] = new_type_env[v]

    def __enter__(self):
        global type_env
        return type_env

    def __exit__(self, type, value, traceback):
        global type_env
        for v in self.env_vars:
            del type_env[v]

# forward declarations for externs
for m in hlir16.objects['Method']:
    # TODO temporary fix for l3-routing-full, this will be computed later on
    with types({
        "T": "struct uint8_buffer_s",
        "O": "unsigned",
        "HashAlgorithm": "int",
    }):
        t = m.type
        ret_type = format_type(t.returnType)
        args = ", ".join([format_expr(arg) for arg in t.parameters.parameters if not arg.type._type_ref('is_metadata')] + ['SHORT_STDPARAMS'])

        #[ extern ${ret_type} ${m.name}(${args});



# TODO do not duplicate code
def unique_stable(items):
    """Returns only the first occurrence of the items in a list.
    Equivalent to unique_everseen from Python 3."""
    from collections import OrderedDict
    return list(OrderedDict.fromkeys(items))

#{ char* action_names[] = {
for table in hlir16.tables:
    for action in unique_stable(table.actions):
        #[ "action_${action.action_object.name}",
#} };


for ctl in hlir16.controls:
    for act in ctl.actions:
        fun_params = ["SHORT_STDPARAMS", "action_{}_params_t parameters".format(act.name)]

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
