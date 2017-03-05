# Copyright 2016 Eotvos Lorand University, Budapest, Hungary
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import sys
import json

from p4_hlir.main import HLIR
from p4_hlir.hlir.p4 import *
from p4_hlir.frontend.dumper import *
from p4_hlir.frontend.semantic_check import *
from p4_hlir.hlir.p4_expressions import *

from p4ast import *

# ==============================================================================

def parse_state(ps):
    ops = []
    for op in ps['parser_ops']:
        if op['op'] == 'extract':
            p = op['parameters'][0]
            # p['type']
            name = p['value']
            ops += [ParserExtract(RefExpression(name))]

    if len(ps['transition_key']) == 0:
        t = ps['transitions'][0]
        next_state = t['next_state']
        if next_state is None:
            next_state = 'ingress'
        next_state = RefExpression(next_state)
        ret = ParserImmediateReturn(next_state)
    else:
        select = []
        for tk in ps['transition_key']:
            if tk['type'] == "field":
                headername = tk['value'][0]
                fieldname = tk['value'][1]
                select += [FieldRefExpression(RefExpression(headername), fieldname)]
            # TODO
            # elif tk['type'] == "latest":
            # elif tk['type'] == "current":
        cases = []
        for t in ps['transitions']:
            next_state = t['next_state']
            if next_state is None:
                next_state = 'ingress'
            next_state = RefExpression(next_state)
            if t['value'] == "default":
                c = ParserSelectDefaultCase(next_state)
            elif t['mask'] is None:
                value = int(t['value'], 16)
                c = ParserSelectCase([(Integer(value),)], next_state)
            else:
                value = int(t['value'], 16)
                mask = int(t['mask'], 16)
                c = ParserSelectCase([(Integer(value),Integer(mask))], next_state)
            cases.append(c)
        ret = ParserSelectReturn(select, cases)
    name = ps['name']
    return ParserFunction(name, ops, ret)

# ------------------------------------------------------------------------------

def table(t):
    name = t['name']
    match_type = t['match_type']
    min_size = Integer(t['min_size']) if 'min_size' in t else None
    max_size = Integer(t['max_size']) if 'max_size' in t else None
    size = Integer(t['size']) if 'size' in t else None
    actionspec = []
    for aname in t['actions']:
        actionspec += [RefExpression(aname)]
    reads = []
    for k in t['key']:
        ref = k['target']
        fieldref = FieldRefExpression(RefExpression(ref[0]), ref[1])
        TableFieldMatch((fieldref,), k['match_type'])
        #TODO TableFieldMatch((fieldref,mask), k['match_type'])
    timeout = None
    return Table(name, actionspec, None, reads, min_size, max_size, size, timeout)

def table_apply(name, tables):
    cases = []
    for t in tables:
        if t['name'] != name:
            continue
        for a in t['next_tables']:
            name2 = t['next_tables'][a]
            if name2 is None:
                continue
            cases += [ControlFunctionApplyActionCase([RefExpression(a)], [table_apply(name2, tables)])]
    return ControlFunctionApplyAndSelect(RefExpression(name), cases)

def control(p):
    body = [table_apply(p['init_table'], p['tables'])]
    o = [ControlFunction(p['name'], body)]
    for t in p['tables']:
        o += [table(t)]
    return o

# ------------------------------------------------------------------------------

def expression(rd, p):
    if p['type'] == "hexstr":
        val = p['value']
        return Integer(int(val, 16))
    elif p['type'] == "field":
        ref = p['value']
        return FieldRefExpression(RefExpression(ref[0]), ref[1])
    elif p['type'] == "runtime_data":
        index = p['value']
        pname = rd[index]['name']
        return RefExpression(pname)
    elif p['type'] == "expression":
        e = p['value']
        if 'type' in e:
            return expression(rd, e)
        else:
            op = e['op']
            left = e['left']
            right = e['right']
            #op = str_ops[op][0]
            return BinaryExpression(op,expression(rd, left), expression(rd, right))

def primitive_action(rd, a):
    name = a['op']
    parameters = []
    for p in a['parameters']:
        parameters.append(expression(rd, p))
    if parameters == []:
        return ActionCall(RefExpression(name))
    else:
        return ActionCallWP(RefExpression(name), parameters)

def action(a):
    name = a['name']
    params = []
    for rd in a['runtime_data']:
        params += [str(rd['name'])]
    body = []
    for pa in a['primitives']:
        body += [primitive_action(a['runtime_data'], pa)]
    return ActionFunction(name, params, body)

# ------------------------------------------------------------------------------

def header_type(ht):
    length = 0
    max_length = 0
    layout = []
    for f in ht['fields']:
        field_name = f[0]
        width = f[1]
        length += width
        # TODO signed, saturating attributes
        layout += [(str(field_name), Integer(width), [])]
    max_length = length
    return HeaderType(ht['name'], layout, Integer(length), Integer(max_length))

# ------------------------------------------------------------------------------

def header_instance(h):
    name = h['name']
    ht = h['header_type']
    if h['metadata']:
        return HeaderInstanceMetadata(ht, name)
    else:
        return HeaderInstanceRegular(ht, name)


def json2hlir(filepath):

    # Loading and processing JSON...

    with open(filepath) as data_file:
        data = json.load(data_file)

    # Creating the P4 objects described in JSON...

    all_p4_objects = []

    for ht in data["header_types"]:
        if ht['name'] == 'standard_metadata_t':
            continue
        all_p4_objects.append(header_type(ht))

    for h in data["headers"]:
        all_p4_objects.append(header_instance(h))

    # for x in data["header_stacks"]:
    # for x in data["field_lists"]:

    for p in data["parsers"]:
        for ps in p["parse_states"]:
            all_p4_objects.append(parse_state(ps))

    # for x in data["deparsers"]:
    # for x in data["meter_arrays"]:
    # for x in data["counter_arrays"]:
    # for x in data["register_arrays"]:
    # for x in data["calculations"]:
    # for x in data["learn_lists"]:

    for a in data["actions"]:
        all_p4_objects.append(action(a))

    for p in data["pipelines"]:
        all_p4_objects += control(p)

    # for x in data["checksums"]:
    # for x in data["force_arith"]:

    # Synthesising the P4 AST...

    p4_program = P4Program("", -1, all_p4_objects)

    with open('src/utils/primitives.json') as data_file:
        primitives = json.load(data_file)

    sc = P4SemanticChecker()
    sc.semantic_check(p4_program, primitives)

    # Translating the P4 AST to HLIR...

    h = HLIR()

    d = P4HlirDumper()
    d.dump_to_p4(h, p4_program, primitives)

    p4_validate(h)
    p4_dependencies(h)
    p4_field_access(h)

    return h
