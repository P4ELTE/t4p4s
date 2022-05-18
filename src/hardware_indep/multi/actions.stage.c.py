# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from compiler_common import get_hdrfld_name, generate_var_name, SugarStyle, make_const

compiler_common.current_compilation['is_multicompiled'] = True

part_count = compiler_common.current_compilation['multi']
multi_idx = compiler_common.current_compilation['multi_idx']

all_ctl_acts = sorted(((ctl, act) for ctl in hlir.controls for act in ctl.actions if len(act.body.components) != 0), key=lambda k: len(k[1].body.components))
ctl_acts = list((ctl, act) for idx, (ctl, act) in enumerate(all_ctl_acts) if idx % part_count == multi_idx)

if ctl_acts == []:
    compiler_common.current_compilation['skip_output'] = True
else:
    #[ #define T4P4S_MULTI_IDX ${multi_idx}
    #[ #include "multi_actions.c"
