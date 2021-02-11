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
#!/usr/bin/env python

from hlir16.p4node import P4Node, deep_copy, get_fresh_node_id

def apply_annotations(postfix, extra_args, expr):
    if expr.methodCall.method.node_type != "PathExpression":
        return expr

    on_error = lambda node_id: addError('transforming hlir16', 'Recursion found during deep-copy on node {}'.format(node_id))

    def make_node(arg):
        node = P4Node()
        node.node_type   = 'Parameter'
        node.type        = arg.type
        node.name        = postfix + "_extra_param"
        node.direction   = 'in'
        node.declid      = 0         #TODO: is this important?
        node.annotations = []
        return node

    extra_args_types = [make_node(arg) for arg in extra_args]

    new_decl = deep_copy(expr.methodCall.method.ref, on_error)
    new_decl.name += "_" + postfix
    new_decl.type.parameters.parameters.vec += extra_args_types
    del new_decl.type.parameters.parameters.vec[:2]

    expr.methodCall.method.ref = new_decl
    expr.methodCall.method.path.name += "_" + postfix
    expr.methodCall.arguments.vec += extra_args
    del expr.methodCall.arguments.vec[:2]

    return x


def search_for_annotations(stmt):
    available_optimization_annotations = ['offload', 'atomic', 'async']

    if stmt.node_type != "BlockStatement":
        return stmt

    annots = [ann for ann in stmt.annotations.annotations if ann.name in available_optimization_annotations]
    name   = '_'.join([ann.name for ann in annots])
    if "" != name:
        args = [ann.expr for ann in annots]
        stmt.components = map(lambda component: apply_annotations(name, args, component), stmt.components)
    return stmt


def transform_hlir16(hlir16):
    pipeline_elements = hlir16.p4_main.arguments

    for pe in pipeline_elements:
        ctl = hlir16.objects.get(pe.expression.type.name, 'P4Control')
        if ctl is not None:
            ctl.body.components = map(search_for_annotations, ctl.body.components)

    return hlir16
