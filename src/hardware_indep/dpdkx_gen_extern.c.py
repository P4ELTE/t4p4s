# SPDX-License-Identifier: Apache-2.0
# Copyright 2021 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_type, get_all_extern_call_infos
from utils.extern import extern_has_tuple_params
from compiler_common import generate_var_name
from more_itertools import unique_everseen

#[ #include "dpdk_lib.h"
#[ #include "util_debug.h"
#[ #include "dpdkx_gen_extern.h"

def by_partypename(callinfo):
    mname_parts, parinfos, ret, partypeinfos = callinfo
    return (mname_parts, partypeinfos)

callinfos = list(unique_everseen(get_all_extern_call_infos(hlir), key=by_partypename))

# # note: generating custom body for digests
# for mname_parts, parinfos, ret, _partypeinfos in callinfos:
#     if mname_parts != 'digest':
#         continue

#     parname, pardir, partype, (partypename, ctype, argtype, argvalue), type_par_idx = parinfos[1]

#     digest_struct = partype

#     #{ struct type_field_list get_type_field_list_${partype.name}() {
#     #[     struct type_field_list fields;
#     #[     fields.fields_quantity = ${len(parinfos)};
#     #[     fields.field_ptrs = malloc(sizeof(uint8_t*)*fields.fields_quantity);
#     #[     fields.field_widths = malloc(sizeof(uint8_t*)*fields.fields_quantity);

#     with_vars = [(idx, fld, generate_var_name(f'digest_field{idx+1}') if fld.node_type != 'Type_Header' else None) for idx, fld in enumerate(digest_struct.fields)]

#     for idx, fld, var in with_vars:
#         if fld.node_type != 'Type_Header':
#             #[    ${format_type(fld.type)} $var;

#     for idx, fld, var in with_vars:
#         if fld.node_type == 'Type_Header':
#             if fld.expr.type.is_metadata:
#                 hdrname, fldname = fld.expr.name,   fld.member
#             else:
#                 hdrname, fldname = fld.expr.member, fld.expression.fld_ref.name

#             #[     fields.field_ptrs[$idx]   = (uint8_t*) get_fld_pointer(pd, FLD($hdrname,$fldname));
#             #[     fields.field_widths[$idx] =            fld_infos[FLD($hdrname,$fldname)].size;
#         else:
#             #[     fields.field_ptrs[$idx]   = (uint8_t*)&$var;
#             #[     fields.field_widths[$idx] = ${fld.urtype.size};

#     #[     return fields;
#     #} }
#     #[


# #[ // forward declarations
# for mname_parts, parinfos, rettype, partypeinfos in callinfos:
#     if mname_parts == 'digest':
#         continue

#     if extern_has_tuple_params(mname_parts, parinfos):
#         continue

#     mname_postfix = ''.join(f',{ptn}' for (ptype, ptn) in partypeinfos)
#     params = ', '.join([ctype for parname, pardir, partype, (partypename, ctype, argtype, argvalue), type_par_idx in parinfos if argvalue != None] + ['SHORT_STDPARAMS'])
#     #[     $rettype EXTERNIMPL${len(partypeinfos)}(${mname_parts}${mname_postfix})($params);


tuple_buf_type = 'uint8_buffer_t'

def is_tuple_param(ptype):
    return ptype.urtype.node_type == 'Type_Struct'

def is_tuple_type(partypeinfo):
    partype, partypename = partypeinfo
    return is_tuple_param(partype)

def replace_tuple_type(ctype, partypeinfos, type_par_idx):
    return tuple_buf_type if type_par_idx is None or is_tuple_type(partypeinfos[type_par_idx]) else ctype

# def coroutine_generate_impl_funs():
#     for mname_parts, parinfos, rettype, partypeinfos in callinfos:
#         if not extern_has_tuple_params(mname_parts, parinfos):
#             continue

#         short_parinfos = [(ctype, partype, parname, pardir) for parname, pardir, partype, (partypename, ctype, argtype, argvalue), type_par_idx in parinfos if argvalue != None]

#         mname_postfix = '' if partypeinfos == () or is_tuple_type(partypeinfos[0]) else f'__{partypeinfos[0][1]}'
#         # mname_arg_postfix = ''.join(f',u8s' if is_tuple_param(ptype) else f',{ptn}' for (ptype, ptn), (ctype, partype, parname, pardir) in zip(list(partypeinfos).get(0, []), short_parinfos))
#         # mname_postfix = ['' for partype, partypename in partypeinfos]
#         # mname_postfix = '' if partypeinfos == () or is_tuple_type(partypeinfos[0]) else f'__{partypeinfos[0][1]}'
#         params_list = [replace_tuple_type(ctype, partypeinfos, type_par_idx) for parname, pardir, partype, (partypename, ctype, argtype, argvalue), type_par_idx in parinfos if argvalue != None]
#         params = ','.join(params_list + ['SHORT_STDPARAMS'])
#         yield f'{mname_parts}{mname_postfix}', params, params_list.count(tuple_buf_type), rettype


# #[ // forward declarations with tuple parameters
# for mname_final, params, tuple_buf_count, rettype in unique_everseen(coroutine_generate_impl_funs()):
#     #[     $rettype EXTERNIMPL1(${mname_final},${tuple_buf_count}xbuf)($params);

# #[


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


# for mname_parts, parinfos, ret, partypeinfos in callinfos:
#     if mname_parts == 'digest':
#         continue

#     if not extern_has_tuple_params(mname_parts, parinfos):
#         continue

#     short_parinfos = [(ctype, partype, parname) for parname, pardir, partype, (partypename, ctype, argtype, argvalue), type_par_idx in parinfos if argvalue != None]

#     pars = [f'{ctype} {parname}' for parname, pardir, partype, (partypename, ctype, argtype, argvalue), type_par_idx in parinfos if argvalue != None] + ['SHORT_STDPARAMS']
#     args = [to_buf(partype, parname) for parname, pardir, partype, (partypename, ctype, argtype, argvalue), type_par_idx in parinfos if argvalue != None] + ['SHORT_STDPARAMS_IN']

#     mname_postfix = ''.join(f',{ptn}' for (ptype, ptn) in partypeinfos)
#     # TODO it can also be i8s if signed
#     mname_arg_postfix = ''.join(f',u8s' if not is_tuple_param(partype) else f',{ptn}' for (ptype, ptn), (ctype, partype, parname) in zip(partypeinfos, short_parinfos))

#     #{ void SHORT_EXTERNCALL${len(partypeinfos)}(${mname_parts}${mname_postfix})(${', '.join(pars)}) {
#     #[     EXTERNIMPL${len(partypeinfos)}(${mname_parts}${mname_arg_postfix})(${', '.join(args)});
#     #} }
#     #[


calls = set()
for mname_parts, parinfos, ret, partypeinfos in callinfos:
    partype_suffix = ''.join(f',{ptn}' for (ptype, ptn) in partypeinfos)
    params = ', '.join([f'{ctype} {parname}' for parname, pardir, partype, (partypename, ctype, argtype, argvalue), type_par_idx in parinfos if ctype is not None] + ['SHORT_STDPARAMS'])
    params_as_buf = ', '.join([f'{to_buf_type(partype, ctype)} {parname}' for parname, pardir, partype, (partypename, ctype, argtype, argvalue), type_par_idx in parinfos if ctype is not None] + ['SHORT_STDPARAMS'])

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
    #{ $ret $call(${",".join(mname_parts)}${partype_suffix})($params) {

    # for refvar, (pardir, partype, argtype, argvalue), parinfo in zip(refvars, arginfos, parinfos):
    #     if refvar is None:
    #         continue

    #     if pardir != 'in':
    #         basetype = parinfo[2]
    #         #[     ${format_type(basetype)} $refvar = $argvalue;
    #     else:
    #         #[     $argtype $refvar = ($argtype)$argvalue;

    #[     ${return_stmt}EXTERNIMPL${partypeinfolen + len(mname_parts)-1}(${",".join(mname_parts)}${mname_postfix_as_buf})(${args_as_buf});
    #} }
    #[
