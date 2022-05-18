# SPDX-License-Identifier: Apache-2.0
# Copyright 2021 Eotvos Lorand University, Budapest, Hungary

from compiler_log_warnings_errors import addWarning
from utils.codegen import to_c_bool

compiler_common.current_compilation['is_multicompiled'] = True

part_count = compiler_common.current_compilation['multi']
multi_idx = compiler_common.current_compilation['multi_idx']

table_names = (table.short_name + ("/keyless" if table.key_bit_size == 0 else "") + ("/hidden" if table.is_hidden else "") for table in hlir.tables)
all_table_infos = sorted(zip(hlir.tables, table_names), key=lambda k: len(k[0].actions))
table_infos = list(ti for idx, ti in enumerate(all_table_infos) if idx % part_count == multi_idx)

all_hdrs = sorted(hlir.header_instances.filterfalse(lambda hdr: hdr.urtype.is_metadata), key=lambda hdr: len(hdr.urtype.fields))
hdrs = list(hdr for idx, hdr in enumerate(all_hdrs) if idx % part_count == multi_idx)

if hdrs == []:
    compiler_common.current_compilation['skip_output'] = True
else:
    #[ #define T4P4S_MULTI_IDX ${multi_idx}
    #[ #include "multi_parser.c"
