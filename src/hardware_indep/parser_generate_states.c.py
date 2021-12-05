# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

import os
import re

parser = hlir.parsers[0]

def fmt_key(key):
    if key.node_type == 'DefaultExpression':
        return '_'
    if key.node_type == 'Constant':
        return f'{key.value:0{key.base//4}x}'
    return f'???#{key.node_type}'

def get_elems(node):
    if node.node_type == 'ListExpression':
        return node.components.map(fmt_key)
    if node.node_type == 'Constant':
        return [fmt_key(node)]
    return []

def output_states(state, known_states):
    new_known_states = set(known_states)
    new_known_states.add(state.Node_ID)

    if state.name == 'accept':
        yield([])
        return

    if state.name == 'reject':
        yield(["rejected"])
        return

    def get_name(comp):
        if comp.node_type == 'Slice':
            return f'{comp.e0.path.name}[{comp.e1.value}:{comp.e2.value}]'
        if 'expr' not in comp:
            return f'{comp.path.name}'
        if 'path' in comp.expr and comp.expr.path.name == "standard_metadata":
            return f'all_metadatas.{comp.member}/{comp.urtype.size}'
        if comp.expr.node_type == 'ArrayIndex':
            return f'{comp.expr.left.expr.path.name}.{comp.expr.left.member}[{comp.expr.right.value}]/{comp.urtype.size}'
        if 'member' in comp.expr:
            return f'{comp.expr.member}.{comp.member}/{comp.urtype.size}'
        return f'{comp.expr.path.name}/{comp.urtype.size}'

    if 'select' in state.selectExpression:
        keys = [get_name(comp) for comp in state.selectExpression.select.components]
        prev_conds = []
        for case in state.selectExpression.selectCases:
            infos = [f'{key}={value}' for key, value in zip(keys, get_elems(case.keyset))]
            next_state_name = case.state.path.name
            next_state = parser.states.get(next_state_name)

            if next_state.Node_ID in new_known_states:
                yield([f'state-already-visited:{state.name}'])
                continue

            for info in output_states(next_state, new_known_states):
                yield(infos + prev_conds + info)

            if len(infos) == 1:
                prev_conds.append(infos[0].replace('=', '≠'))
            elif len(infos) > 1:
                joined_infos = ' '.join(infos)
                prev_conds.append(f'NOT({joined_infos})')
    else:
        next_state_name = state.selectExpression.path.name
        next_state = parser.states.get(next_state_name)

        if next_state.Node_ID in new_known_states:
            yield([f'state-already-visited:{state.name}'])
            return

        for info in output_states(next_state, known_states):
            yield(info)


infos = output_states(parser.states.get('start'), set())

cuco = compiler_common.current_compilation
targetdir = os.path.dirname(cuco['to'])

def simplify_infos(txts):
    if any('NOT' in txt for txt in txts):
        return txts

    def is_ok(txt, keys):
        key, *rest = txt.split("≠")
        return rest == [] or key not in keys

    keys = set(txt.split("=")[0] for txt in txts)
    return [txt for txt in txts if is_ok(txt, keys)]

short_infos = [simplify_infos(info) for info in infos if info != []]

if short_infos != []:
    with open(os.path.join(targetdir, '..', 'parser_state_transitions.txt'), 'w', encoding='utf8') as file:
        for info in short_infos:
            file.write(' '.join(info) + '\n')
