# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_expr, format_type
from compiler_common import get_hdrfld_name, generate_var_name, SugarStyle, make_const

compiler_common.current_compilation['is_multicompiled'] = True

part_count = compiler_common.current_compilation['multi']
multi_idx = compiler_common.current_compilation['multi_idx']

all_ctl_acts = sorted(((ctl, act) for ctl in hlir.controls for act in ctl.actions if len(act.body.components) != 0), key=lambda k: len(k[1].body.components))
ctl_acts = list((ctl, act) for idx, (ctl, act) in enumerate(all_ctl_acts) if idx % part_count == multi_idx)

if ctl_acts == []:
    compiler_common.current_compilation['skip_output'] = True
else:
    from compiler_log_warnings_errors import addError, addWarning
    from utils.codegen import format_declaration, format_statement, format_expr, format_type, get_method_call_env
    from compiler_common import types, unique_everseen

    #[ #include <unistd.h>

    #[ #include "dpdk_lib.h"
    #[ #include "actions.h"
    #[ #include "util_debug.h"
    #[ #include "util_packet.h"

    #[ #include "util_packet.h"

    #[ extern const char* action_names[];
    #[ extern const char* action_canonical_names[];
    #[ extern const char* action_short_names[];

    #[ extern ctrl_plane_backend bg;

    for mcall in hlir.all_nodes.by_type('MethodCallStatement').map('methodCall').filter(lambda n: 'path' in n.method and n.method.path.name=='digest'):
        digest = mcall.typeArguments[0]
        funname = f'{mcall.method.path.name}__{digest.path.name}'

        #[ extern ${format_type(mcall.urtype)} $funname(uint32_t /* ignored */ receiver, ctrl_plane_digest cpd, SHORT_STDPARAMS);

    #[ extern void do_assignment(header_instance_t dst_hdr, header_instance_t src_hdr, SHORT_STDPARAMS);

    ################################################################################

    for ctl, act in ctl_acts:
        name = act.annotations.annotations.get('name')
        if name:
            #[     // action name: ${name.expr[0].value}
        #{     void action_code_${act.name}(action_${act.name}_params_t parameters, SHORT_STDPARAMS) {
        if len(act.body.components) != 0:
            #[         uint32_t value32, res32;
            #[         (void)value32, (void)res32;
            #[         control_locals_${ctl.name}_t* local_vars = (control_locals_${ctl.name}_t*) pd->control_locals;

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
        #}     }
        #[
