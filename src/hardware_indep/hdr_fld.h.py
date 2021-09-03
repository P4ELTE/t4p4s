# SPDX-License-Identifier: Apache-2.0
# Copyright 2021 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_type
from compiler_common import unique_everseen

#[ #pragma once

#[ #define HDRT(hdrt)  HDRT_ ## hdrt



# for hdr in unique_everseen(hdr for struct in hlir.news.data for hdr in struct.fields.map('urtype') if hdr.node_type == 'Type_Header'):
for hdr in unique_everseen(hlir.headers.map('urtype').filterfalse('is_metadata')):
    #{ typedef struct {
    for fld in hdr.fields:
        #[     ${format_type(fld.type, addon=fld.name)};
    #} } __attribute__((packed)) HDRT(${hdr.urtype.name});
    #[

for struct in hlir.news.data:
    name = re.sub('_t$', '', struct.name)

    #{ typedef struct {
    for field in struct.fields:
        #[     ${format_type(field.urtype, field.name)};
    #} } __attribute__((packed)) ${name}_t;
    #[
