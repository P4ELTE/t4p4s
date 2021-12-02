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


for typedef in hlir.typedefs:
    #[ typedef ${format_type(typedef.type)} ${typedef.name};
#[
