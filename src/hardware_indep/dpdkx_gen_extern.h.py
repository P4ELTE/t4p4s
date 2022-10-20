# SPDX-License-Identifier: Apache-2.0
# Copyright 2021 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_type, get_detailed_extern_callinfos
from utils.extern import extern_has_tuple_params
from compiler_common import generate_var_name, SugarStyle
from more_itertools import unique_everseen
from hlir16.hlir_model import smem_types_by_model

#[ #include "dpdk_lib.h"
#[ #include "util_debug.h"
#[ #include "stateful_memory_type.h"


detailed_callinfos = get_detailed_extern_callinfos(hlir)

def gen_get_externtype(part, partypeinfolen, partype_suffix, extra_suffix_as_buf, arginfos):
    get_smem, reverse_get_smem = smem_types_by_model(hlir)

    if part == get_smem['register']:
        reg = arginfos[0][1]
        sign = '' if reg.isSigned else 'u'
        #[ REGTYPE(${sign}int,${reg.padded_size})*
    elif part in reverse_get_smem:
        #[ SMEMTYPE(${reverse_get_smem[part]})*
    else:
        #[ EXTERNTYPE$partypeinfolen($part${extra_suffix_as_buf}${partype_suffix})*

def gen_get_impl_externtype(part, partypeinfolen, extrainfolen, arginfos):
    get_smem, reverse_get_smem = smem_types_by_model(hlir)

    if part == get_smem['register']:
        reg = arginfos[0][1]
        sign = '' if reg.isSigned else 'u'
        #[ REGTYPE(${sign}int,${reg.padded_size})*
    elif part in reverse_get_smem:
        #[ SMEMTYPE(${reverse_get_smem[part]})*
    else:
        #[ EXTERNTYPE0($part)*


smem_types = set(hlir.smem_insts.map('smem').map('smem_type'))

for mname_parts, partype_suffix, partypeinfolen, extra_suffix, extra_suffix_as_buf, extrainfolen, params, params_as_buf, ret, mname_postfix, mname_postfix_as_buf, args, args_as_buf, refvars, arginfos, parinfos in sorted(unique_everseen(detailed_callinfos, key=lambda c: c[0:5])):
    if mname_parts[0] in smem_types:
        continue

    if len(mname_parts) == 1:
        call = f'SHORT_EXTERNCALL{partypeinfolen + len(mname_parts)-1}'
    else:
        call = f'EXTERNCALL{partypeinfolen + len(mname_parts)-2}'
        varname = generate_var_name('extern')
        with SugarStyle("inline_comment"):
            # externtype = gen_get_externtype(mname_parts[0], partypeinfolen, partype_suffix, extra_suffix_as_buf)
            externtype = gen_get_externtype(mname_parts[0], partypeinfolen-extrainfolen, partype_suffix, '', arginfos)
        params = f'{externtype} {varname}, {params}'
        args_as_buf = f'{varname}, {args_as_buf}'

    return_stmt = '' if ret != 'void' else 'return '
    if len(arginfos) > 0 and arginfos[0][1].node_type == 'Type_Struct':
        ifdef_suffix = partype_suffix.replace(',', '_')
        #[ #ifdef externdef_${mname_parts[0]}${ifdef_suffix}
    #[ $ret $call(${",".join(mname_parts)}${extra_suffix}${partype_suffix})($params);
    if len(arginfos) > 0 and arginfos[0][1].node_type == 'Type_Struct':
        #[ #endif

#[


smem_types = set(hlir.smem_insts.map('smem').map('smem_type'))
for mname_parts, partype_suffix, partypeinfolen, extra_suffix, extra_suffix_as_buf, extrainfolen, params, params_as_buf, ret, mname_postfix, mname_postfix_as_buf, args, args_as_buf, refvars, arginfos, parinfos in sorted(unique_everseen(detailed_callinfos, key=lambda c: (c[0:4], c[6]))):
    if mname_parts[0] in smem_types:
        continue

    if len(mname_parts) == 1:
        call = f'SHORT_EXTERNCALL{partypeinfolen + len(mname_parts)-1}'
    else:
        call = f'EXTERNCALL{partypeinfolen + len(mname_parts)-2}'
        varname = generate_var_name('extern')
        with SugarStyle("inline_comment"):
            externtype = gen_get_externtype(mname_parts[0], partypeinfolen-extrainfolen, partype_suffix, '', arginfos)
            impl_externtype = gen_get_impl_externtype(mname_parts[0], partypeinfolen, extrainfolen, arginfos)
        params_as_buf = f'{externtype} {varname}, {params_as_buf}'

    #[ $ret EXTERNIMPL${partypeinfolen + len(mname_parts) -1}(${",".join(mname_parts)}${mname_postfix_as_buf})(${params_as_buf});

#[
