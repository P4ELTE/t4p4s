# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

import os
import re

from itertools import takewhile, dropwhile

NOT_separator = '▶'

for parser in hlir.parsers:
    def fmt_key(key):
        if key.node_type == 'DefaultExpression':
            return '_'
        if key.node_type == 'Constant':
            return f'{key.value}' if key.value < 10 else f'{key.value:0{(key.urtype.size+3)//4}x}'
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
                infos = [f'{key}={value}' for key, value in zip(keys, get_elems(case.keyset)) if value != '_']
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
                    keys = set(re.sub(r"[.].*", "", info) for info in infos)
                    if len(keys) == 1:
                        joined_infos = NOT_separator.join(re.sub(r"[^.]*[.]", ".", info) for info in infos)
                        prev_conds.append(f'{list(keys)[0]}.NOT({joined_infos})')
                    else:
                        joined_infos = NOT_separator.join(infos)
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
        def is_ok(txt, keys):
            key, *rest = txt.split("≠")
            return rest == [] or key not in keys

        keys = set(txt.split("=")[0] for txt in txts if 'NOT' not in txt)
        return [txt for txt in txts if 'NOT' in txts or is_ok(txt, keys)]

    short_infos = [simplify_infos(info) for info in infos if info != []]

    if short_infos != []:
        with open(os.path.join(targetdir, '..', f'parser_state_transitions_{parser.name}_v1.txt'), 'w', encoding='utf8') as file:
            file.write('accepted: \n')
            for info in (info for info in short_infos if 'rejected' not in info):
                file.write('    ' + ' '.join(info) + '\n')
            file.write('\n')

            if len(list((info for info in short_infos if 'rejected' in info))) != 0:
                file.write('rejected: \n')
                for info in (info for info in short_infos if 'rejected' in info):
                    file.write('    ' + ' '.join(info) + '\n')

    # v2
    if short_infos != []:
        with open(os.path.join(targetdir, '..', f'parser_state_transitions_{parser.name}_v2.txt'), 'w', encoding='utf8') as file:
            file.write('accepted: \n')
            collected = []
            for info in (info for info in short_infos if 'rejected' not in info):
                out_info = info
                for idx in range(len(info), 1, -1):
                    pre = info[:idx]
                    if pre in collected:
                        collected_idx = collected.index(pre)
                        out_info = [f'@{collected_idx+1:04}'] + simplify_infos(info[idx:])
                        break

                this_except_last = info[:-1]
                collected.append(this_except_last)

                file.write(f'    {len(collected):04} ' + ' '.join(out_info) + '\n')
            file.write('\n')

            if len(list((info for info in short_infos if 'rejected' in info))) != 0:
                file.write('rejected: \n')
                for info in (info for info in short_infos if 'rejected' in info):
                    file.write('    ' + ' '.join(info) + '\n')

    # v3
    if short_infos != []:
        with open(os.path.join(targetdir, '..', f'parser_state_transitions_{parser.name}_v3.txt'), 'w', encoding='utf8') as file:
            file.write('accepted: \n')
            collected = {}
            curr_idx = 1
            for info in (info for info in short_infos if 'rejected' not in info):
                out_info = info
                for idx in range(len(info), 1, -1):
                    pre = tuple(info[:idx])
                    if pre in collected:
                        collected_idx = collected[pre]
                        out_info = [f'@{collected_idx}'] + simplify_infos(info[idx:])
                        break
                    else:
                        collected[tuple(pre)] = f'{curr_idx:04}-{len(info)+1-idx}'

                file.write(f'    {curr_idx:04} ' + ' '.join(out_info) + '\n')
                curr_idx = curr_idx + 1
            file.write('\n')

            if len(list((info for info in short_infos if 'rejected' in info))) != 0:
                file.write('rejected: \n')
                for info in (info for info in short_infos if 'rejected' in info):
                    file.write('    ' + ' '.join(info) + '\n')


    def final_simplify_infos(in_infos, is_reject, postfixes):
        prefixes = {}
        curr_idx = 1
        infos2 = []
        for info in (info for info in in_infos if ('rejected' in info) == is_reject):
            if tuple(info) in prefixes:
                continue

            out_info = info
            for idx in range(len(info), 1, -1):
                pre = tuple(info[:idx])
                if pre in prefixes:
                    prefixes_idx = prefixes[pre]
                    out_info = [f'@{prefixes_idx}'] + simplify_infos(info[idx:])
                    break
                else:
                    prefixes[tuple(pre)] = f'{curr_idx:04}+{idx}-{len(info)+1-idx}'

            infos2.append(out_info)
            curr_idx = curr_idx + 1

        infos3 = []
        postfix_idx = len(postfixes) + 1
        for info in infos2:
            if '@' not in info[0] or len(info) <= 2:
                infos3.append(info)
                continue

            postfix = tuple(info[1:])
            if postfix not in postfixes:
                postfixes[postfix] = postfix_idx
                postfix_idx = postfix_idx + 1

            infos3.append([info[0], f'post@{postfixes[postfix]:04}'])

        return infos3

    def not_prefix_len(txts):
        return -len(list(takewhile(lambda txt: all(not txt.startswith(pre) for pre in ('@', 'post@')), txts)))

    def info_sorter(infos):
        return sorted(enumerate(infos), key=lambda keyval: (not_prefix_len(keyval[1]), len(''.join(keyval[1])), keyval[0]))

    # v4
    if short_infos != []:
        postfixes = {}
        accepts = final_simplify_infos(short_infos, False, postfixes)
        rejects = final_simplify_infos(short_infos, True, postfixes)

        with open(os.path.join(targetdir, '..', f'parser_state_transitions_{parser.name}_v4.txt'), 'w', encoding='utf8') as file:
            file.write('postfixes: \n')
            for postfix, idx in sorted(postfixes.items(), key=lambda keyval: (-len(keyval[0]), -len(''.join(keyval[0])), keyval[1])):
                file.write(f'    post@{idx:04} {" ".join(postfix)}\n')
            file.write('\n')

            file.write('accepted: \n')
            for idx, info in info_sorter(accepts):
                file.write(f'    {idx+1:04} {" ".join(info)}\n')
            file.write('\n')

            if len(list((info for info in short_infos if 'rejected' in info))) != 0:
                file.write('rejected: \n')
                for idx, info in info_sorter(rejects):
                    file.write(f'    {idx+1:04} {" ".join(info)}\n')
