#[ // SPDX-License-Identifier: Apache-2.0
#[ // Copyright 2019 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_type

#[ #pragma once

#[ #include <stdint.h>
#[ #include "parser.h"


def short_name(name):
    return name[:-2] if name.endswith('_t') else name

for err in hlir.errors:
    name = short_name(err.c_name)
    #{ typedef enum enum_${name}_s {
    for member in err.members:
        #[     ${member.c_name},
    #} } ${name}_t;

for enum in hlir.enums:
    name = short_name(enum.c_name)
    #{ typedef enum enum_${name}_s {
    for m in enum.members:
        #[     ${m.c_name},
    #} } ${name}_t;
#[


# TODO can the filter condition be simpler?
for struct in hlir.news.data.filter(lambda n: not any(t.node_type == 'Type_Header' for t in n.fields.map('urtype'))):
    name = re.sub('_t$', '', struct.name)

    #{ typedef struct ${name}_s {
    for field in struct.fields:
        #[     ${format_type(field.urtype, field.name)};
    #} } ${name}_t;


for typedef in hlir.typedefs:
    #[ typedef ${format_type(typedef.type)} ${typedef.name};
