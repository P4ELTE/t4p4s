# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_declaration, format_statement, format_expr, format_type, gen_format_type, get_method_call_env
from compiler_log_warnings_errors import addError, addWarning
from compiler_common import types, generate_var_name, get_hdrfld_name, unique_everseen

#[ #include "dataplane_impl.h"

for hdr in hlir.header_instances:
    #{ void update_hdr_${hdr.name}(STDPARAMS) {
    for fld in hdr.urtype.fields:
        if fld.preparsed or fld.urtype.size > 32:
            continue

        #{     if (pd->fields.FLD_ATTR(${hdr.name},${fld.name}) == MODIFIED) {
        #[         MODIFY_INT32_INT32_AUTO_PACKET(pd, HDR(${hdr.name}), FLD(${hdr.name},${fld.name}), pd->fields.FLD(${hdr.name},${fld.name}));
        #}     }
    #} }
    #[

#{ void update_packet(STDPARAMS) {
for hdr in hlir.header_instances:
    #[     update_hdr_${hdr.name}(STDPARAMS_IN);
#} }
