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
        extern_name = f'EXTERNTYPE({extern.name})'

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
