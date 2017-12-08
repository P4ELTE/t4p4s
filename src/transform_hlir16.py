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

def apply_annotations(postfix, extra_args, x):
    if (x.methodCall.method.node_type=="PathExpression") :
	x.methodCall.method.ref.name += "_" + postfix
        x.methodCall.method.path.name += "_" + postfix
        x.methodCall.arguments.vec += extra_args
    return x

def search_for_annotations(x):
    available_optimization_annotations = ['offload']

    if (x.node_type == "BlockStatement") :
        name_list = [annot.name for annot in x.annotations.annotations.vec if annot.name in  available_optimization_annotations]
        arg_list = [annot.expr for annot in x.annotations.annotations.vec if annot.name in  available_optimization_annotations]
        if ([] != name_list) :
	    x.components = map(lambda x : apply_annotations('_'.join(name_list), arg_list, x), x.components)
    return x

def transform_hlir16(hlir16):

    main = hlir16.declarations['Declaration_Instance'][0] # TODO what if there are more package instances?
    pipeline_elements = main.arguments 
	
    for pe in pipeline_elements:
        c = hlir16.declarations.get(pe.type.name, 'P4Control')
        if c is not None:
            c.body.components = map(search_for_annotations, c.body.components)



