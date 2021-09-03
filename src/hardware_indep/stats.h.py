#[ // SPDX-License-Identifier: Apache-2.0
#[ // Copyright 2021 Eotvos Lorand University, Budapest, Hungary

from itertools import chain

from compiler_common import unique_everseen
from utils.codegen import format_type

targets_parser       = (f'parser state {s.name}' for s in hlir.parsers[0].states)
targets_table_apply  = (f'table apply {table.name}' for table in hlir.tables)
targets_table_hit    = (f'table apply {table.name}' for table in hlir.tables if 'key' in table)
targets_table_miss   = (f'table apply {table.name}' for table in hlir.tables if 'key' in table)
targets_table_action = (f'action {table.name} {action_name}' for table in hlir.tables for action_name in table.actions.map('expression.method.path.name'))

targets = list(target.split(' ') for target in chain(targets_parser, targets_table_apply, targets_table_hit, targets_table_miss, targets_table_action))
part1, part2, part3 = zip(*targets)
stat1, stat2, stat3 = sorted(unique_everseen(part1)), sorted(unique_everseen(part2)), sorted(unique_everseen(part3))


#[ #pragma once

#[ #define T4STAT(part1, part2, part3)   t4stat_ ## part1 ## _ ## part2 ## _ ## part3
#[ #define T4REQ(part1, part2, part3)   t4req_ ## part1 ## _ ## part2 ## _ ## part3

#{ typedef enum {
for s1 in stat1:
    #[     t4stat1_$s1,
#} } t4p4s_stat1_e;
#[

#{ typedef enum {
for s2 in stat2:
    #[     t4stat2_$s2,
#} } t4p4s_stat2_e;
#[

#{ typedef enum {
for s3 in stat3:
    #[     t4stat3_$s3,
#} } t4p4s_stat3_e;
#[


#{ typedef struct {
parser = hlir.parsers[0]
for s in parser.states:
    #[     bool T4STAT(parser,state,${s.name});

#[

for table in hlir.tables:
    #[     bool T4STAT(table,apply,${table.name});

    if 'key' in table:
        #[     bool T4STAT(table,hit,${table.name});
        #[     bool T4STAT(table,miss,${table.name});
    else:
        #[     bool T4STAT(table,used,${table.name});


    for action_name in table.actions.map('expression.method.path.name'):
        #[     bool T4STAT(action,${table.name},${action_name});

#} } t4p4s_stats_t;


#{ typedef enum {
#[     req_none,
parser = hlir.parsers[0]
for s in parser.states:
    #[     T4REQ(parser,state,${s.name}),

#[

for table in hlir.tables:
    #[     T4REQ(table,apply,${table.name}),
    if 'key' in table:
        #[     T4REQ(table,hit,${table.name}),
        #[     T4REQ(table,miss,${table.name}),
    else:
        #[     T4REQ(table,used,${table.name}),


    for action_name in table.actions.map('expression.method.path.name'):
        #[     T4REQ(action,${table.name},${action_name}),

#} } t4p4s_controlflow_name_e;
#[
