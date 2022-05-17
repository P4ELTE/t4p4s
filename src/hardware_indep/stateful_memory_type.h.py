# SPDX-License-Identifier: Apache-2.0
# Copyright 2021 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_type
from hlir16.hlir_model import model_specific_infos, packets_by_model

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

# TODO remove
#{ typedef struct {
#[     rte_atomic32_t value;
#} } EXTERNTYPE1(Checksum,u32);

pars = [(extern, ctor, m, par) for extern in hlir.externs for ctor in extern.constructors for m in ctor.env_node.interface_methods if m is not None for par in m.type.parameters.parameters[:1] if par.urtype.node_type == 'Type_Struct']
# print('dbgpars', pars)
# breakpoint()
for extern, ctor, m, par in pars:
    #{ typedef struct {
    for fld in par.type.fields:
        #[     ${format_type(fld.type, addon=fld.name)};
    #} } EXTERNTYPE1(${extern.name},${par.type.name});
    #[

