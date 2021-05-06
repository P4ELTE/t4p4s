# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_declaration, format_statement, format_expr, format_type, gen_format_type, get_method_call_env
from compiler_log_warnings_errors import addError, addWarning
from compiler_common import types, generate_var_name, get_hdrfld_name, unique_everseen

#[ #pragma once

#[ #include "dataplane_impl.h"
#[ #include "gen_model.h"

# TODO make this an import from hardware_indep
#[ #include "dpdk_smem.h"

all_ctl_stages = ((ctl, idx, comp) for ctl in hlir.controls for idx, comp in enumerate(ctl.body.components))

for table in hlir.tables:
    #[ apply_result_t ${table.name}_apply(STDPARAMS);

for ctl, idx, comp in all_ctl_stages:
    #[ void control_stage_${ctl.name}_${idx}(control_locals_${ctl.name}_t* local_vars, STDPARAMS);
