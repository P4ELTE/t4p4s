#[ // SPDX-License-Identifier: Apache-2.0
#[ // Copyright 2019 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_type

#[ #pragma once

#[ #include <stdbool.h>
#[ #include <stdint.h>
#[ #include "parser.h"


#[ #define NB_TABLES ${len(hlir.tables)}


def short_name(name):
    return name[:-2] if name.endswith('_t') else name

for err in hlir.errors:
    name = short_name(err.c_name)
    #{ typedef enum {
    for member in err.members:
        #[     ${member.c_name},
    #} } ${name}_t;

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


for typedef in hlir.typedefs:
    #[ typedef ${format_type(typedef.type)} ${typedef.name};


#{ #ifdef T4P4S_STATS
#{ typedef struct {
parser = hlir.parsers[0]
for s in parser.states:
    #[     bool parser_state__${s.name};

#[

for table in hlir.tables:
    #[         bool table_apply__${table.name};

    if 'key' in table:
        #[         bool table_hit__${table.name};
        #[         bool table_miss__${table.name};
    else:
        #[         bool table_used__${table.name};


    for action_name in table.actions.map('expression.method.path.name'):
        #[         bool table_action_used__${table.name}_${action_name};

#} } t4p4s_stats_t;
#} #endif
