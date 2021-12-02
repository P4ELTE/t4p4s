# SPDX-License-Identifier: Apache-2.0
# Copyright 2021 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_type, get_all_extern_call_infos
from utils.extern import extern_has_tuple_params
from compiler_common import generate_var_name
from more_itertools import unique_everseen

#[ #include "dpdk_lib.h"
#[ #include "util_debug.h"

callinfos = get_all_extern_call_infos(hlir)

# for mname_parts, parinfos, ret, partypenames in unique_everseen(callinfos):
#     partype_suffix = ''.join(f',{partypename}' for partype, partypename in partypenames)
#     params = ', '.join([f'{ctype} {parname}' for parname, pardir, partype, (partypename, ctype, argtype, argvalue), type_par_idx in parinfos] + ['SHORT_STDPARAMS'])
#     call = f'SHORT_EXTERNCALL{len(partypenames)}'

#     #[ $ret $call($mname_parts${partype_suffix})($params);
# #[


# def is_tuple_param(partype):
#     return partype.urtype.node_type == 'Type_Struct'

# def to_buf_type(partype, is_const):
#     if is_tuple_param(partype):
#         return f'uint8_buffer_t'
#     return format_type(partype, is_const=is_const)

# calls = set()
# impls = set()
# for mname_parts, parinfos, ret, partypenames in callinfos:
#     short_parinfos = [(ctype, partype, parname, pardir) for parname, pardir, partype, (partypename, ctype, argtype, argvalue), type_par_idx in parinfos if argvalue != None]

#     pars = [f'{ctype} {parname}' for ctype, partype, parname, pardir in short_parinfos] + ['SHORT_STDPARAMS']
#     argtypes = [to_buf_type(partype, pardir not in ('out', 'inout')) for ctype, partype, parname, pardir in short_parinfos] + ['SHORT_STDPARAMS']

#     mname_postfix = ''.join(f',{ptn}'  for (ptype, ptn), (ctype, partype, parname, pardir) in zip(partypenames, short_parinfos))
#     # TODO it can also be i8s if signed
#     mname_arg_postfix = ''.join(f',u8s' if is_tuple_param(ptype) else f',{ptn}' for (ptype, ptn), (ctype, partype, parname, pardir) in zip(partypenames, short_parinfos))

#     impls.add((mname_parts, len(partypenames), mname_arg_postfix, ', '.join(argtypes)))
#     calls.add((mname_parts, len(partypenames), mname_postfix, ', '.join(pars)))

# for mname_parts, partypelen, mname_arg_postfix, argtypes_txt in sorted(unique_everseen(impls)):
#     #[ void EXTERNIMPL${partypelen}(${mname_parts}${mname_arg_postfix})(${argtypes_txt});
# #[

# for mname_parts, partypelen, mname_postfix, pars_txt in sorted(unique_everseen(calls)):
#     #[ void SHORT_EXTERNCALL${partypelen}(${mname_parts}${mname_postfix})(${pars_txt});
# #[


# TODO the following bit is a duplicate from the .c.py file

def is_tuple_param(partype):
    return partype.urtype.node_type == 'Type_Struct'


def to_buf_param(partype, parname):
    size = f'({"+".join(f"{fld.size}" for fld in partype.urtype.fields)}+7) / 8'

    offset = '0'
    offsets = []
    lens = []
    for fld in partype.urtype.fields:
        lens.append(f'{fld.size}')
        offsets.append(offset)
        offset += f'+{fld.size}'

    parnames = ", ".join(f'"{parname}"' for parname in partype.fields.map('name'))

    components = [
        ('size', f'{size}'),
        ('buffer', f'(uint8_t*){parname}'),
        ('part_count', f'{len(partype.fields)}'),
        ('part_bit_offsets', f'{{{", ".join(offsets)}}}'),
        ('part_bit_sizes', f'{{{", ".join(lens)}}}'),
        ('name', f'"{partype.name}"'),
        ('part_names', f'{{{parnames}}}'),
    ]

    return f'(uint8_buffer_t){{{", ".join(f".{component} = {value}" for component, value in components)}}}'


def to_buf(partype, parname):
    if is_tuple_param(partype):
        return to_buf_param(partype, parname)
    return parname

def to_buf_type(partype, partypename, buftype='u8s'):
    return buftype if is_tuple_param(partype) else partypename

calls = set()
for mname_parts, parinfos, ret, partypeinfos in callinfos:
    partype_suffix = ''.join(f',{ptn}' for (ptype, ptn) in partypeinfos)
    params = ', '.join([f'{ctype} {parname}' for parname, pardir, partype, (partypename, ctype, argtype, argvalue), type_par_idx in parinfos if ctype is not None] + ['SHORT_STDPARAMS'])
    params_as_buf = ', '.join([f'{to_buf_type(partype, ctype, "uint8_buffer_t")} {parname}' for parname, pardir, partype, (partypename, ctype, argtype, argvalue), type_par_idx in parinfos if ctype is not None] + ['SHORT_STDPARAMS'])

    arginfos = tuple((pardir, partype, argtype, argvalue) for parname, pardir, partype, (partypename, ctype, argtype, argvalue), type_par_idx in parinfos if argvalue != None)
    refvars = tuple(argvalue if pardir in ('out', 'inout') else None for idx, (pardir, partype, argtype, argvalue) in enumerate(arginfos))

    args = ', '.join([refvar if refvar is not None else argvalue for refvar, (pardir, partype, argtype, argvalue) in zip(refvars, arginfos)] + ['SHORT_STDPARAMS_IN'])
    args_as_buf = ', '.join([refvar if refvar is not None else to_buf(partype, argvalue) for refvar, (pardir, partype, argtype, argvalue) in zip(refvars, arginfos)] + ['SHORT_STDPARAMS_IN'])
    mname_postfix = ''.join(f',{ptn}' for (ptype, ptn) in partypeinfos)
    mname_postfix_as_buf = ''.join(f',{to_buf_type(ptype, ptn)}' for (ptype, ptn) in partypeinfos)

    calls.add((len(partypeinfos), mname_parts, partype_suffix, params, params_as_buf, ret, mname_postfix, mname_postfix_as_buf, args, args_as_buf, refvars, arginfos, parinfos))



for partypeinfolen, mname_parts, partype_suffix, params, params_as_buf, ret, mname_postfix, mname_postfix_as_buf, args, args_as_buf, refvars, arginfos, parinfos in sorted(unique_everseen(calls, key=lambda c: c[0:3])):
    if len(mname_parts) == 1:
        call = f'SHORT_EXTERNCALL{partypeinfolen + len(mname_parts)-1}'
    else:
        call = f'EXTERNCALL{partypeinfolen + len(mname_parts)-2}'
        extern_type_name = f''
        varname = generate_var_name('extern')
        params = f'EXTERNTYPE({mname_parts[0]})* {varname}, ' + params
        args_as_buf = f'{varname}, ' + args_as_buf

    return_stmt = '' if ret != 'void' else 'return '
    #[ $ret $call(${",".join(mname_parts)}${partype_suffix})($params);

#[


for partypeinfolen, mname_parts, partype_suffix, params, params_as_buf, ret, mname_postfix, mname_postfix_as_buf, args, args_as_buf, refvars, arginfos, parinfos in sorted(unique_everseen(calls, key=lambda c: (c[0:2], c[4]))):
    if len(mname_parts) == 1:
        call = f'SHORT_EXTERNCALL{partypeinfolen + len(mname_parts)-1}'
    else:
        call = f'EXTERNCALL{partypeinfolen + len(mname_parts)-2}'
        extern_type_name = f''
        varname = generate_var_name('extern')
        params_as_buf = f'EXTERNTYPE({mname_parts[0]})* {varname}, ' + params_as_buf
        args_as_buf = f'{varname}, ' + args_as_buf

    #[ $ret EXTERNIMPL${partypeinfolen + len(mname_parts)-1}(${",".join(mname_parts)}${mname_postfix_as_buf})(${params_as_buf});

#[
