# SPDX-License-Identifier: Apache-2.0
# Copyright 2021 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_type, get_detailed_extern_callinfos
from utils.extern import extern_has_tuple_params
from compiler_common import generate_var_name
from more_itertools import unique_everseen

#[ #include "dpdk_lib.h"
#[ #include "util_debug.h"


detailed_callinfos = get_detailed_extern_callinfos(hlir)

def get_externtype(part, partypeinfolen, partype_suffix, varname):
    return f'EXTERNTYPE{partypeinfolen}({part}{partype_suffix})* {varname}'

for partypeinfolen, mname_parts, partype_suffix, params, params_as_buf, ret, mname_postfix, mname_postfix_as_buf, args, args_as_buf, refvars, arginfos, parinfos in sorted(unique_everseen(detailed_callinfos, key=lambda c: c[0:3])):
    if len(mname_parts) == 1:
        call = f'SHORT_EXTERNCALL{partypeinfolen + len(mname_parts)-1}'
    else:
        call = f'EXTERNCALL{partypeinfolen + len(mname_parts)-2}'
        varname = generate_var_name('extern')
        externtype = get_externtype(mname_parts[0], partypeinfolen, partype_suffix, varname)
        params = f'{externtype}, {params}'
        args_as_buf = f'{varname}, ' + args_as_buf

    return_stmt = '' if ret != 'void' else 'return '
    #[ $ret $call(${",".join(mname_parts)}${partype_suffix})($params);

#[


for partypeinfolen, mname_parts, partype_suffix, params, params_as_buf, ret, mname_postfix, mname_postfix_as_buf, args, args_as_buf, refvars, arginfos, parinfos in sorted(unique_everseen(detailed_callinfos, key=lambda c: (c[0:2], c[4]))):
    if len(mname_parts) == 1:
        call = f'SHORT_EXTERNCALL{partypeinfolen + len(mname_parts)-1}'
    else:
        call = f'EXTERNCALL{partypeinfolen + len(mname_parts)-2}'
        varname = generate_var_name('extern')
        externtype = get_externtype(mname_parts[0], partypeinfolen, partype_suffix, varname)
        params_as_buf = f'{externtype}, {params_as_buf}'
        args_as_buf = f'{varname}, ' + args_as_buf

    #[ $ret EXTERNIMPL${partypeinfolen + len(mname_parts)-1}(${",".join(mname_parts)}${mname_postfix_as_buf})(${params_as_buf});

#[
