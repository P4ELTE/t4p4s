# SPDX-License-Identifier: Apache-2.0
# Copyright 2021 Eotvos Lorand University, Budapest, Hungary

from compiler_common import unique_everseen, generate_var_name, get_hdr_name, get_hdrfld_name
from utils.codegen import format_expr, format_type, gen_format_slice

import os

#[ #pragma once

#[ #include "dpdk_lib.h"
#[ #include "tables.h"

for table in hlir.tables:
    #[ void ${table.name}_set_default_table_action(${table.name}_action_t* action, const char* action_name, p4_action_parameter_t** action_params);
