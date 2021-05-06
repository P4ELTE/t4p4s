#[ // SPDX-License-Identifier: Apache-2.0
#[ // Copyright 2019 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_type

#[ #pragma once

#[ #include <stdbool.h>
#[ #include <stdint.h>
#[ #include "parser.h"
#[ #include "stats.h"

#[ #include "dataplane.h"
#[ #include "util_packet.h"


#[ #define NB_TABLES ${len(hlir.tables)}


def short_name(name):
    return name[:-2] if name.endswith('_t') else name


for ee in hlir.errors + hlir.enums:
    name = short_name(ee.c_name)
    #[ #define T4P4S_TYPE_${name}

for data in hlir.news.data:
    #[ #define T4P4S_TYPE_${data.name}


for err in hlir.errors:
    name = short_name(err.c_name)
    #{ typedef enum {
    for member in err.members:
        #[     ${member.c_name},
    #} } ${name}_t;
#[

for enum in hlir.enums:
    name = short_name(enum.c_name)
    #{ typedef enum {
    for m in enum.members:
        #[     ${m.c_name},
    #} } ${name}_t;
#[


# TODO can the filter condition be simpler?
for struct in hlir.news.data.filter(lambda n: not any(t.node_type == 'Type_Header' for t in n.fields.map('urtype'))):
    name = re.sub('_t$', '', struct.name)

    #{ typedef struct {
    for field in struct.fields:
        #[     ${format_type(field.urtype, field.name)};
    #} } ${name}_t;
    #[

for typedef in hlir.typedefs:
    #[ typedef ${format_type(typedef.type)} ${typedef.name};
#[

#[ void do_assignment(header_instance_t dst_hdr, header_instance_t src_hdr, SHORT_STDPARAMS);
#[ void set_hdr_valid(header_instance_t hdr, SHORT_STDPARAMS);
#[ void set_hdr_invalid(header_instance_t hdr, SHORT_STDPARAMS);
#[
