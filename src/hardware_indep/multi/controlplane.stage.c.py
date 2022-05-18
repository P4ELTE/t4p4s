# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

compiler_common.current_compilation['is_multicompiled'] = True

part_count = compiler_common.current_compilation['multi']
multi_idx = compiler_common.current_compilation['multi_idx']

all_tables = sorted(hlir.tables, key=lambda table: len(table.actions))
tables = list(table for idx, table in enumerate(all_tables) if idx % part_count == multi_idx)

if tables == []:
    compiler_common.current_compilation['skip_output'] = True
else:
    #[ #define T4P4S_MULTI_IDX ${multi_idx}
    #[ #include "multi_controlplane.c"
