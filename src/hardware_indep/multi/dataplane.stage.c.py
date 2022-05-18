# SPDX-License-Identifier: Apache-2.0
# Copyright 2021 Eotvos Lorand University, Budapest, Hungary

compiler_common.current_compilation['is_multicompiled'] = True

part_count = compiler_common.current_compilation['multi']
multi_idx = compiler_common.current_compilation['multi_idx']

table_names = (table.short_name + ("/keyless" if table.key_bit_size == 0 else "") + ("/hidden" if table.is_hidden else "") for table in hlir.tables)
all_table_infos = sorted(zip(hlir.tables, table_names), key=lambda k: len(k[0].actions))
table_infos = list(ti for idx, ti in enumerate(all_table_infos) if idx % part_count == multi_idx)

all_ctl_stages = ((ctl, idx, comp) for ctl in hlir.controls for idx, comp in enumerate(ctl.body.components))
ctl_stages = list(cic for idx, cic in enumerate(all_ctl_stages) if (idx + len(all_table_infos)) % part_count == multi_idx)

if table_infos == [] and ctl_stages == []:
    compiler_common.current_compilation['skip_output'] = True
else:
    #[ #define T4P4S_MULTI_IDX ${multi_idx}
    #[ #include "multi_dataplane.c"
