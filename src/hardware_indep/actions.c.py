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
from utils.codegen import format_declaration_16, format_statement_16_ctl
import math


#[ #include "dpdk_lib.h"
#[ #include "actions.h"
#[ #include <unistd.h>

#[ extern ctrl_plane_backend bg;



#[ // TODO create this meter properly
#[ void teid_meters(packet_descriptor_t* pd, lookup_table_t** tables) {
#[ }


for ctl in hlir16.controls:
    for act in ctl.actions:
        fun_params = ["packet_descriptor_t* pd", "lookup_table_t** tables"]
        if act.parameters.parameters.vec != []:
            fun_params += ["struct action_{}_params parameters".format(act.name)]

        #{ void action_code_${act.name}(${', '.join(fun_params)}) {
        #[ uint32_t value32, res32, mask32;
        #[ (void)value32; (void)res32; (void)mask32;
        #[ control_locals_${ctl.name}_t control_locals = *(control_locals_${ctl.name}_t*) pd->control_locals;

        for stmt in act.body.components:
            global statement_buffer
            statement_buffer = ""

            code = format_statement_16_ctl(stmt, ctl)
            if statement_buffer != "":
                #= statement_buffer
                statement_buffer = ""
            #= code
        #} }
