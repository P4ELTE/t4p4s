#[ // SPDX-License-Identifier: Apache-2.0
#[ // Copyright 2019 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_type
from compiler_common import unique_everseen

#[ #pragma once

#[ #include <stdbool.h>
#[ #include <stdint.h>
#[ #include "parser.h"
#[ #include "stats.h"

#[ #include "dataplane.h"
#[ #include "util_packet.h"


#[ #define T4P4S_BROADCAST_PORT 100


#[ #define NB_TABLES ${len(hlir.tables)}


def short_name(name):
    return name[:-2] if name.endswith('_t') else name


for ee in hlir.errors + hlir.enums:
    name = short_name(ee.c_name)
    #[ #define T4P4S_TYPE_${name}

for data in hlir.news.data:
    #[ #define T4P4S_TYPE_${data.name}


for ee in hlir.errors + hlir.enums:
    kind = 'enum' if ee.node_type == 'Type_Enum' else 'error'
    name = short_name(ee.c_name)
    #{ typedef enum {
    for m in ee.members:
        #[     ${m.c_name},
    #} } ${name}_t;
    #[
    #[ extern const char* ${kind}_value_names_${ee.name}[${len(ee.members)}];
    #[
#[


for typedef in hlir.typedefs:
    #[ typedef ${format_type(typedef.type)} ${typedef.name};
#[
