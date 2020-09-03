#!/usr/bin/env python

# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from hlir16.p4node import P4Node, deep_copy, get_fresh_node_id

def apply_annotations(postfix, extra_args, expr):
    if expr.methodCall.method.node_type != "PathExpression":
        return expr

    on_error = lambda node_id: addError('transforming hlir', f'Recursion found during deep-copy on node {node_id}')

    def make_node(arg):
        if arg.is_vec():
            return P4Node(arg.vec)

        node = P4Node({'node_type': 'Parameter'})
        node.type        = arg.type
        node.name        = f"{postfix}_extra_param"
        node.direction   = 'in'
        node.annotations = []
        return node

    extra_args_types = [make_node(arg) for arg in extra_args]

    new_decl = deep_copy(expr.methodCall.method.ref, on_error=on_error)
    new_decl.name += f"_{postfix}"
    new_decl.type.parameters.parameters.vec += extra_args_types
    del new_decl.type.parameters.parameters.vec[:2]

    expr.methodCall.method.ref = new_decl
    expr.methodCall.method.path.name += f"_{postfix}"
    expr.methodCall.arguments.vec += extra_args
    del expr.methodCall.arguments.vec[:2]

    return expr


def search_for_annotations(stmt):
    available_optimization_annotations = ['offload', 'atomic']

    if stmt.node_type != "BlockStatement":
        return stmt

    annots = [ann for ann in stmt.annotations.annotations if ann.name in available_optimization_annotations]
    name   = '_'.join([ann.name for ann in annots])
    if "" != name:
        args = [ann.expr for ann in annots]
        stmt.components = P4Node([apply_annotations(name, args, comp) for comp in stmt.components])
    return stmt


def transform_hlir(hlir):
    pipeline_elements = hlir.news.main.arguments

    for pe in pipeline_elements:
        ctl = hlir.controls.get(pe.expression.type.name)
        if ctl is not None:
            ctl.body.components = P4Node([search_for_annotations(comp) for comp in ctl.body.components])

    return hlir
