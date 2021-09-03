# SPDX-License-Identifier: Apache-2.0
# Copyright 2021 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import get_all_extern_call_infos
from compiler_common import unique_everseen

#[ #include "dpdk_lib.h"
#[ #include "util_debug.h"

callinfos = get_all_extern_call_infos(hlir)

for mname, parinfos, ret, partypenames in unique_everseen(callinfos):
    partype_suffix = ''.join(f'__{partypename}' for partypename in partypenames)
    params = ', '.join([f'{ctype} {parname}' for parname, pardir, partype, (partypename, ctype, argtype, argvalue) in parinfos] + ['SHORT_STDPARAMS'])

    #[ $ret $mname${partype_suffix}($params);
    #[
