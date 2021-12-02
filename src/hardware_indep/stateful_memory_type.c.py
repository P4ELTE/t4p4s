# SPDX-License-Identifier: Apache-2.0
# Copyright 2021 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_type
from hlir16.hlir_model import model_specific_infos

#[ #pragma once

#[ #include <stdbool.h>
#[ #include <stdint.h>

#[ #include "aliases.h"


def extern_repr_is_model_specific(extern):
    return extern.name in model_specific_infos[hlir.news.model]['extern_reprs']


for extern in hlir.externs.filter(lambda extern: len(extern.constructors) > 0 or extern_repr_is_model_specific(extern)):
    if extern.repr is None:
        continue

    if 'smem_type' in (smem := extern):
        struct_name = f'SMEMTYPE({smem.smem_type}_s)'
    else:
        struct_name = f'EXTERNTYPE({extern.name}_s)'

    #{ struct ${struct_name} {
    #[     ${format_type(extern.repr, addon='value', is_atomic=True)};
    #[     #ifdef T4P4S_DEBUG
    #[         char name[256];
    #[     #endif
    #} };
    #[
