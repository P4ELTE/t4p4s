# SPDX-License-Identifier: Apache-2.0
# Copyright 2021 Eotvos Lorand University, Budapest, Hungary

from compiler_log_warnings_errors import addError
from utils.codegen import format_type, get_short_type, get_detailed_extern_callinfos
from hlir16.hlir_model import model_specific_infos, packets_by_model
from more_itertools import unique_everseen

#[ #pragma once

#[ #include <stdbool.h>
#[ #include <stdint.h>

#[ #include "aliases.h"

#{ typedef enum {
#[     pob_packets,
#[     pob_bytes,
#[     pob_packets_and_bytes,
#} } packets_or_bytes_e;


def extern_repr_is_model_specific(extern):
    return extern.name in model_specific_infos[hlir.news.model]['extern_reprs']


for extern in hlir.externs.filter(lambda extern: (len(extern.constructors) > 0 or extern_repr_is_model_specific(extern)) and extern.repr is not None):
    if 'smem_type' in (smem := extern):
        extern_name = f'SMEMTYPE({smem.smem_type})'
    else:
        extern_name = f'EXTERNTYPE0({extern.name})'

    #{ typedef struct {
    if 'smem_type' in extern:
        #[     ${format_type(smem.repr, addon='packets', is_atomic=True)};
        #[     ${format_type(smem.repr, addon='bytes', is_atomic=True)};
        #[     packets_or_bytes_e pob;
    else:
        #[     ${format_type(extern.repr, addon='value', is_atomic=True)};
    #[     #ifdef T4P4S_DEBUG
    #[         char name[256];
    #[     #endif
    #} } ${extern_name};
    #[

already_generateds = set()

pars = [(extern, ctor, m, par) for extern in hlir.externs for ctor in extern.constructors for m in ctor.env_node.interface_methods if m is not None for par in m.type.parameters.parameters[:1] if par.urtype.node_type == 'Type_Struct']
for extern, ctor, m, par in pars:
    already_generateds.add((extern.name, par.type.name))

    #[ #define externdef_${extern.name}${par.type.name}
    #{ typedef struct {
    for fld in par.type.fields:
        #[     ${format_type(fld.type, addon=fld.name)};
    #} } EXTERNTYPE1(${extern.name},${par.type.name});
    #[

def get_extern_params(infotuple):
    def replace_arg(arg):
        argut = arg.urtype
        nt = argut.node_type
        if nt == 'Type_Bits':
            return (nt, argut.isSigned, argut.size)
        if nt == 'Type_InfInt':
            return (nt)
        if nt == 'Type_Struct':
            return (nt, argut.name)
        return arg

    _, extern = infotuple
    externtype = extern.type.baseType.urtype
    return (externtype, tuple(extern.type.arguments.map(replace_arg)))

spec_externs = hlir.groups.pathexprs.specialized_canonical.filter('type.baseType.node_type', 'Type_Extern')
spec_extern_types = set((se.path.name, se.urtype.name) for se in spec_externs)

spec_extern_infos = list(unique_everseen(((local.urtype.name, local) for ctl in hlir.controls for local in ctl.controlLocals if local.node_type == 'Declaration_Instance' if (local.name, local.urtype.name) in spec_extern_types), key=get_extern_params))

# TODO can there be conflict between two similar locals from two separate controls?
for externname, local in spec_extern_infos:
    typepars = local.type.arguments.map('urtype').map(get_short_type)

    typeinfo = tuple([externname] + list(typepars))
    if typeinfo in already_generateds:
        continue

    comma_suffix = ''.join(typepars.map(lambda tp: f',{tp}'))
    underscore_suffix = ''.join(typepars.map(lambda tp: f'_{tp}'))

    repr = local.type.baseType.urtype.repr

    #[ #define externdef_$externname${underscore_suffix}
    #{ typedef struct {
    if repr is None:
        addError('Determining extern representation', f'Cannot find the representation for extern {local.type.baseType.urtype.name}')
        #[     int value; // placeholder representation for extern ${local.type.baseType.urtype.name}
    else:
        #[     ${format_type(repr, addon='value')};
    # TODO other fields?
    #} } EXTERNTYPE${len(typepars)}($externname${comma_suffix});
    #[


detailed_callinfos = get_detailed_extern_callinfos(hlir)
for mname_parts, partype_suffix, partypeinfolen, extra_suffix, extra_suffix_as_buf, extrainfolen, params, params_as_buf, ret, mname_postfix, mname_postfix_as_buf, args, args_as_buf, refvars, arginfos, parinfos in sorted(unique_everseen(detailed_callinfos, key=lambda c: c[0:3])):
    if len(arginfos) > 0 and arginfos[0][1].node_type == 'Type_Struct':
        ifdef_suffix = partype_suffix.replace(',', '_')
        #[ #define externdef_${mname_parts[0]}${ifdef_suffix}

#[ // TODO remove; this is just a placeholder
#[ typedef struct {
#[     rte_atomic16_t value /* codegen:114 */;
#[     #ifdef T4P4S_DEBUG
#[     char name[256];
#[     #endif
#[ } EXTERNTYPE1(InternetChecksum,tuple_0);
#[
