#[ // SPDX-License-Identifier: Apache-2.0
#[ // Copyright 2021 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_type

#[ #pragma once

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


#{ typedef enum {
#[      req_none,
parser = hlir.parsers[0]
for s in parser.states:
    #[      req_parser_state__${s.name},

#[

for table in hlir.tables:
    #[          req_table_apply__${table.name},
    if 'key' in table:
        #[         req_table_hit__${table.name},
        #[         req_table_miss__${table.name},
    else:
        #[         req_table_used__${table.name},


    for action_name in table.actions.map('expression.method.path.name'):
        #[         req_table_action_used__${table.name}_${action_name},

#} } t4p4s_controlflow_name_t;
#[

