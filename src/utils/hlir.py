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
#Utility functions for analyzing HLIR and for generating some code parts from HLIR

from p4_hlir.hlir.p4_tables import p4_match_type
import re
import p4_hlir.hlir.p4 as p4
from misc import addError


def format_p4_node(node):
    if node is None:
        return ""
    elif isinstance(node, p4.p4_table):
        return "return apply_table_%s(pd, tables);" % node.name
    elif isinstance(node, p4.p4_conditional_node):
        return "if (%s) { %s } else { %s }" % (format_expr(node.condition), format_p4_node(node.next_[True]), format_p4_node(node.next_[False]))

def format_op(op):
    if op is "not":
        return "!"
    elif op is "and":
        return "&&"
    elif op is "or":
        return "||"
    else:
        return str(op)

def format_expr(e):
    if e is None:
        return ""
    elif isinstance(e, bool):
        return "1" if e else "0"
    elif isinstance(e, int):
        return str(e)
    elif isinstance(e, p4.p4_field):
        return "GET_INT32_AUTO(pd, %s)"%fld_id(e)
    elif isinstance(e, p4.p4_expression):
        if e.op is 'valid' and isinstance(e.right, p4.p4_header_instance):
            return "pd->headers[%s].pointer != NULL" % hdr_prefix(e.right.name)
        else:
            if e.left is None:
                return format_op(e.op) + "(" + format_expr(e.right) + ")"
            else:
                return "(" + format_expr(e.left) + ")" + format_op(e.op) + "(" + format_expr(e.right) + ")"
    else:
        addError("pretty-printing an expression", "expression type %s is not supported yet"%type(e))
        return ""



def hdr_name(name): return re.sub(r'\[([0-9]+)\]', r'_\1', name)

def hdr_prefix(name): return re.sub(r'\[([0-9]+)\]', r'_\1', "header_instance_"+name)
def fld_prefix(name): return "field_instance_"+name

def hstack(name): return re.find(r'\[([0-9]+)\]', name)

def fld_id(f):
    return fld_prefix(hdr_name(f.instance.name) + "_" + f.name)

def instances4stack(hlir, s):
    stack_instances = filter(lambda i: i.max_index > 0 and not i.virtual and i.base_name is s, hlir.p4_header_instances.values())
    return map(lambda i: hdr_prefix(i.name), stack_instances)

def header_stack_ids(hlir):
    stack_instances = filter(lambda i: i.max_index > 0 and not i.virtual, hlir.p4_header_instances.values())
    stack_base_names = list(set(map(lambda i: i.base_name, stack_instances)))
    return map(lambda n: "header_stack_" + n, stack_base_names)

def stack_instance_ids(hlir):
    return map(hdr_prefix, filter(hstack, hlir.p4_header_instances))

def header_instances(hlir): 
    return filter(lambda i: not i.virtual, hlir.p4_header_instances.values())

def header_instance_ids(hlir):
    x = filter(lambda i: not i.virtual, hlir.p4_header_instances.values())
    return map(lambda i: hdr_prefix(i.name), x)

def field_instance_ids(hlir):
    names = [hdr_name(hi.name)+"_"+fn for hi in header_instances(hlir) for fn,fw in hi.header_type.layout.items()]
    return map(fld_prefix, names)

def field_offsets(header_type):
    offsets = []
    o = 0
    for name,length in header_type.layout.items():
        offsets.append(o)
        o += length
    return offsets

def field_mask(bitoffset, bitwidth):
    if bitoffset+bitwidth > 32: return hex(0) # FIXME do something about this
    pos = bitoffset;
    mask = 0;
    while pos < bitoffset + bitwidth:
        mask |= (1 << pos)
        pos += 1
    return hex(mask)


simplePrimitives = set(["drop", "no_op"])
# TODO: better condition...
def notPrimitive(action):
    return ((action.signature_flags == {}) and (not (action.name in simplePrimitives)))

def userActions(actions):
    useractions = []
    for key in actions.keys():
        val = actions[key]
        if notPrimitive(val):
            useractions += [key]
    return useractions

def getTypeAndLength(table) : 
   key_length = 0
   lpm = 0
   ternary = 0
   for field, typ, mx in table.match_fields:
       if typ == p4_match_type.P4_MATCH_TERNARY:
           ternary = 1
       elif typ == p4_match_type.P4_MATCH_LPM:
           lpm += 1          
       key_length += field.width
   if (ternary) or (lpm > 1):
      table_type = "LOOKUP_TERNARY"
   elif lpm:
      table_type = "LOOKUP_LPM"
   else:
      table_type = "LOOKUP_EXACT"
   return (table_type, (key_length+7)/8)
