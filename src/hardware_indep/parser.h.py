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
import inspect
from utils.hlir import *

def enum(es):
    return "enum %s {\n  %s\n};\n\n" % (inspect.stack()[1][3], ",\n  ".join(es))
def harray(t, es):
    es2 = [ a + " /* " + b + " */" for (a,b) in zip(es, header_instance_ids(hlir))]
    return "static const %s %s[HEADER_INSTANCE_COUNT] = {\n  %s\n};\n\n" % (t, inspect.stack()[1][3], ",\n  ".join(es2))
def farray(t, es):
    es2 = [ a + " /* " + b + " */" for (a,b) in zip(es, field_instance_ids(hlir))]
    return "static const %s %s[FIELD_INSTANCE_COUNT] = {\n  %s\n};\n\n"  % (t, inspect.stack()[1][3], ",\n  ".join(es2))
def hsarray(t, es):
    es2 = [ a + " /* " + b + " */" for (a,b) in zip(es, header_stack_ids(hlir))]
    return "static const %s %s[HEADER_STACK_COUNT] = {\n  %s\n};\n\n" % (t, inspect.stack()[1][3], ",\n  ".join(es2))
def hsarrayarray(t, es):
    es2 = [ a + " /* " + b + " */" for (a,b) in zip(es, header_stack_ids(hlir))]
    return "static const %s %s[HEADER_STACK_COUNT][10] = {\n  %s\n};\n\n" % (t, inspect.stack()[1][3], ",\n  ".join(es2))

#==============================================================================

def header_stack_e():
    l = header_stack_ids(hlir)
    l.append("header_stack_")
    return enum(l)

def header_instance_e():
    return enum(header_instance_ids(hlir))

def field_instance_e():
    return enum(field_instance_ids(hlir))

def header_stack_elements():
    stack_instances = filter(lambda i: i.max_index > 0 and not i.virtual, hlir.p4_header_instances.values())
    hs_names = list(set(map(lambda i: i.base_name, stack_instances)))
    xs = map(lambda s : "{" + ",\n  ".join(instances4stack(hlir, s)) + "}", hs_names)
    return hsarrayarray("header_instance_t", xs)

def header_stack_size():
    stack_instances = filter(lambda i: i.max_index > 0 and not i.virtual, hlir.p4_header_instances.values())
    hs_names = list(set(map(lambda i: i.base_name, stack_instances)))
    xs = map(lambda s : str(len(instances4stack(hlir, s))), hs_names)
    return hsarray("unsigned", xs)

def header_instance_byte_width():
    ws = [str(sum([f[1] for f in hi.header_type.layout.items()])/8) for hi in header_instances(hlir)]
    return harray("int", ws)

def header_instance_is_metadata():
    ms = ["1" if hi.metadata else "0" for hi in header_instances(hlir)]
    return harray("int", ms)

def field_instance_bit_width():
    ws = [str(f[1]) for hi in header_instances(hlir) for f in hi.header_type.layout.items()]
    return farray("int", ws)

def field_instance_bit_offset():
    os = [str(item%8) for sublist in [field_offsets(hi.header_type) for hi in header_instances(hlir)] for item in sublist]
    return farray("int", os)

def field_instance_byte_offset_hdr():
    os = [str(item//8) for sublist in [field_offsets(hi.header_type) for hi in header_instances(hlir)] for item in sublist]
    return farray("int", os)

def field_instance_mask():
    ws = [f[1] for hn,hi in hlir.p4_header_instances.items() for f in hi.header_type.layout.items()]
    os = [item%8 for sublist in [field_offsets(hi.header_type) for hi in header_instances(hlir)] for item in sublist]
    ms = [field_mask(a, b) for (a,b) in zip(os, ws)]
    return farray("uint32_t", ms)

def field_instance_header():
    names = [i.name for i in header_instances(hlir) for fn,fw in i.header_type.layout.items()]
    ids = map(hdr_prefix, names)
    return farray("header_instance_t", ids)

sc = len(header_stack_ids(hlir))
hc = len(header_instance_ids(hlir))
fc = len(field_instance_ids(hlir))

#[ #ifndef __HEADER_INFO_H__
#[ #define __HEADER_INFO_H__
#[ 
#[ #define HEADER_INSTANCE_COUNT ${hc}
#[ #define HEADER_STACK_COUNT ${sc}
#[ #define FIELD_INSTANCE_COUNT ${fc}
#[ 
#[ ${ header_stack_e() }
#[ ${ header_instance_e() }
#[ ${ field_instance_e() }
#[ typedef enum header_instance_e header_instance_t;
#[ typedef enum field_instance_e field_instance_t;
#[ ${ header_instance_byte_width() }
#[ ${ header_instance_is_metadata() }
#[ ${ field_instance_bit_width() }
#[ ${ field_instance_bit_offset() }
#[ ${ field_instance_byte_offset_hdr() }
#[ ${ field_instance_mask() }
#[ ${ field_instance_header() }
#[ 
#[ ${ header_stack_elements() }
#[ ${ header_stack_size() }
#[ typedef enum header_stack_e header_stack_t;
#[ 
#[ #endif // __HEADER_INFO_H__
