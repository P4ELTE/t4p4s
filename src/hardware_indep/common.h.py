#[ // SPDX-License-Identifier: Apache-2.0
#[ // Copyright 2019 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_type

#[ #pragma once

#[ #include <stdint.h>
#[ #include "parser.h"

for typedef in hlir.typedefs:
    #[ typedef ${format_type(typedef.type)} ${typedef.name};

for struct in hlir.news.data:
    name = re.sub('_t$', '', struct.name)

    #{ typedef struct ${name}_s {
    for field in struct.fields:
        #[ ${format_type(field.urtype)} ${field.name};
    #} } ${name}_t;
