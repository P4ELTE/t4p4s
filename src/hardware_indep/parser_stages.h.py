# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_declaration, format_statement, format_expr, format_type, gen_format_type, get_method_call_env
from compiler_log_warnings_errors import addError, addWarning
from compiler_common import types, generate_var_name, get_hdrfld_name, unique_everseen

#[ #pragma once

#[ #include "dpdk_lib.h"
#[ #include "util_packet.h"

for hdr in sorted(hlir.header_instances.filterfalse(lambda hdr: hdr.urtype.is_metadata), key=lambda hdr: len(hdr.urtype.fields)):
    #[ int parser_extract_${hdr.name}(uint32_t vwlen, STDPARAMS);

#[ void cannot_parse_hdr(const char* varwidth_txt, const char* hdr_name, uint32_t hdrlen, STDPARAMS);
