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
from hlir16.hlir16_attrs import get_main

def apply_annotations(postfix, extra_args, x):
    if (x.methodCall.method.node_type=="PathExpression") :
        new_decl = deep_copy(x.methodCall.method.ref)
        new_decl.name += "_" + postfix
        extra_args_types = [ P4Node({'id' : get_fresh_node_id(),
                                   'node_type' : 'Parameter',
                                   'type' : arg.type,
                                   'name' : postfix + "_extra_param",
                                   'direction' : 'in',
                                   'declid' : 0,         #TODO: is this important?
                                   'annotations' : []
                                   }) for arg in extra_args ]
        new_decl.type.parameters.parameters.vec += extra_args_types
        x.methodCall.method.ref = new_decl
        x.methodCall.method.path.name += "_" + postfix
        x.methodCall.arguments.vec += extra_args
        del new_decl.type.parameters.parameters.vec[:2]
        del x.methodCall.arguments.vec[:2]
    return x

def search_for_annotations(x):
    available_optimization_annotations = ['offload']

    if (x.node_type == "BlockStatement") :
        name_list = [annot.name for annot in x.annotations.annotations.vec if annot.name in available_optimization_annotations]
        arg_list = []
        for annot in x.annotations.annotations.vec :
            if annot.name in available_optimization_annotations : arg_list += annot.expr
        if ([] != name_list) :
	    x.components = map(lambda x : apply_annotations('_'.join(name_list), arg_list, x), x.components)
    return x

def transform_hlir16(hlir16):
    main = get_main(hlir16)
    pipeline_elements = main.arguments 
	
    for pe in pipeline_elements:
        c = hlir16.declarations.get(pe.type.name, 'P4Control')
        if c is not None:
            c.body.components = map(search_for_annotations, c.body.components)
