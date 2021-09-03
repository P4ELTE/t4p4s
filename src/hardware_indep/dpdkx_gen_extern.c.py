# SPDX-License-Identifier: Apache-2.0
# Copyright 2021 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_type, get_all_extern_call_infos
from compiler_common import generate_var_name
from more_itertools import unique_everseen

#[ #include "dpdk_lib.h"
#[ #include "util_debug.h"

def by_partypename(callinfo):
    mname, parinfos, ret, partypenames = callinfo
    return (mname, partypenames)

callinfos = list(unique_everseen(get_all_extern_call_infos(hlir), key=by_partypename))

# note: generating custom body for digests
for mname, parinfos, ret, _partypenames in callinfos:
    if mname != 'digest':
        continue

    parname, pardir, partype, (partypename, ctype, argtype, argvalue) = parinfos[1]

    digest_struct = partype

    #{ struct type_field_list get_type_field_list_${partype.name}() {
    #[     struct type_field_list fields;
    #[     fields.fields_quantity = ${len(parinfos)};
    #[     fields.field_ptrs = malloc(sizeof(uint8_t*)*fields.fields_quantity);
    #[     fields.field_widths = malloc(sizeof(uint8_t*)*fields.fields_quantity);

    with_vars = [(idx, fld, generate_var_name(f'digest_field{idx+1}') if fld.node_type != 'Type_Header' else None) for idx, fld in enumerate(digest_struct.fields)]

    for idx, fld, var in with_vars:
        if fld.node_type != 'Type_Header':
            #[    ${format_type(fld.type)} $var;

    for idx, fld, var in with_vars:
        if fld.node_type == 'Type_Header':
            if fld.expr.type.is_metadata:
                hdrname, fldname = fld.expr.name,   fld.member
            else:
                hdrname, fldname = fld.expr.member, fld.expression.fld_ref.name

            #[     fields.field_ptrs[$idx]   = (uint8_t*) get_fld_pointer(pd, FLD($hdrname,$fldname));
            #[     fields.field_widths[$idx] =            fld_infos[FLD($hdrname,$fldname)].size;
        else:
            #[     fields.field_ptrs[$idx]   = (uint8_t*)&$var;
            #[     fields.field_widths[$idx] = ${fld.urtype.size};

    #[     return fields;
    #} }
    #[


#[ // forward declarations
for mname, parinfos, ret, partypenames in callinfos:
    if mname == 'digest':
        continue

    # TODO
    rettype = 'void'

    params = ', '.join([argtype for parname, pardir, partype, (partypename, ctype, argtype, argvalue) in parinfos if argvalue != None] + ['SHORT_STDPARAMS'])
    #[     $rettype ${mname}_impl($params);

#[


for mname, parinfos, ret, partypenames in callinfos:
    if mname == 'digest':
        continue

    partype_suffix = ''.join(f'__{partypename}' for partypename in partypenames)
    params = ', '.join([f'{ctype} {parname}' for parname, pardir, partype, (partypename, ctype, argtype, argvalue) in parinfos if ctype is not None] + ['SHORT_STDPARAMS'])

    #{ $ret $mname${partype_suffix}($params) {
    #[     debug("    : Called extern " T4LIT($mname${partype_suffix},extern) "\n");

    arginfos = [(pardir, argtype, argvalue) for parname, pardir, partype, (partypename, ctype, argtype, argvalue) in parinfos if argvalue != None]
    refvars = [generate_var_name(f'extern_arg{idx}') if pardir in ('out', 'inout') else None for idx, (pardir, argtype, argvalue) in enumerate(arginfos)]
    for refvar, (pardir, argtype, argvalue) in zip(refvars, arginfos):
        if refvar is not None:
            # TODO this generates an extra pointer (at least in some cases), get rid of it

            #[     $argtype $refvar = ($argtype)$argvalue;

    args = ', '.join([f'&{refvar}' if refvar is not None else argvalue for refvar, (pardir, argtype, argvalue) in zip(refvars, arginfos)] + ['SHORT_STDPARAMS_IN'])
    #[     ${mname}_impl($args);

    #} }
    #[
