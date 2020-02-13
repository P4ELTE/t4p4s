#[ // SPDX-License-Identifier: Apache-2.0
#[ // Copyright 2019 Eotvos Lorand University, Budapest, Hungary

from hlir16.utils_hlir16 import *
from utils.codegen import format_type

#[ #ifndef __COMMON_H__
#[ #define __COMMON_H__

for typedef in hlir16.objects['Type_Typedef']:
    #[ typedef ${format_type(typedef.type)} ${typedef.name};

for struct in hlir16.objects['Type_Struct']:
    # TODO make condition less arbitrary
    if not struct.name.endswith('_t'):
        continue

    #{ typedef struct ${struct.name[:-2]}_s {
    for field in struct.fields:
        #[ ${format_type(field.type)} ${field.name};
    #} } ${struct.name};

#[ #endif // __COMMON_H__
