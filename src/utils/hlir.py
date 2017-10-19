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

# TODO note that this current implementation is true for all fields modified by at least one primitive action;
#   this should be refined in the future to reflect the intent of selecting frequently accessed field instances only
def is_parsed_field(hlir, field):
    # TODO is this hack really necessary?
    # if True:
    #     return False

    if field.instance.metadata or is_vwf(field) or field.width > 32: return False
    for fun in userActions(hlir):
        for call in fun.call_sequence:
            act = call[0]
            args = call[1]
            if act.name in ["modify_field", "add_to_field"]:
                dst = args[0]
                src = args[1]
                if dst == field or src == field:
                    return True
    return False

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

def valid_expression(hi):
    if isinstance(hi, p4.p4_field): hi = hi.instance
    return p4.p4_expression(None, "valid", hi)

def resolve_field_ref(hlir, hi, exp):
    if exp.op=="valid": return exp
    if isinstance(exp.left, p4.p4_expression):
        resolve_field_ref(hlir, hi, exp.left)
    elif isinstance(exp.left, str):
        exp.left = hlir.p4_fields[hi.name + "." + exp.left]
    if isinstance(exp.right, p4.p4_expression):
        resolve_field_ref(hlir, hi, exp.right)
    elif isinstance(exp.right, str):
        exp.right = hlir.p4_fields[hi.name + "." + exp.right]
    return exp


def hdr_name(name): return re.sub(r'\[([0-9]+)\]', r'_\1', name)

def hdr_prefix(name): return re.sub(r'\[([0-9]+)\]', r'_\1', "header_instance_"+name)
def fld_prefix(name): return "field_instance_"+name

def hstack(name): return re.find(r'\[([0-9]+)\]', name)

def fld_id(f):
    return fld_prefix(hdr_name(f.instance.name) + "_" + f.name)

# TEMPORARY TODO remove this after action.c.py has been upgraded to hlir16
def hdr_fld_id(f):
    hname = hdr_name(f.instance.name)
    return "header_instance_{}, field_{}_t_{}".format(hname, hname, f.name)

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


def variable_width_field_ids(hlir):
    var_ids = []
    for hi in header_instances(hlir):
        field_id = "-1 /* fixed-width header */"
        for name,length in hi.header_type.layout.items():
            if length == p4.P4_AUTO_WIDTH:
                field_id = fld_prefix(hdr_name(hi.name)+"_"+name)
                break
        var_ids.append(field_id)
    return var_ids


def field_offsets(header_type):
    offsets = []
    o = 0
    for name,length in header_type.layout.items():
        offsets.append(o)
        if length != p4.P4_AUTO_WIDTH:
            o += length
        else:
            fixed_length = sum([f[1] if f[1] != p4.P4_AUTO_WIDTH else 0 for f in header_type.layout.items()])
            o += (-fixed_length) % 8
    return offsets

def field_mask(bitoffset, bitwidth):
    if bitwidth == p4.P4_AUTO_WIDTH: return "0x0 /* variable-width field */"
    if bitoffset+bitwidth > 32: return hex(0) # FIXME do something about this
    pos = bitoffset;
    mask = 0;
    while pos < bitoffset + bitwidth:
        d,m = divmod(pos, 8)
        mask |= (1 << (8*d+7-m))
        pos += 1
    return hex(mask)


# TODO: better condition...
def primitive(action):
    return action.signature_flags != {} and action.name not in ["drop", "no_op"]


def userActions(hlir):
    return filter(lambda act: not primitive(act), hlir.p4_actions.values())


def getTypeAndLength(table):
    key_length = 0
    lpm = 0
    ternary = 0
    for field, typ, mx in table.match_fields:
        if typ == p4_match_type.P4_MATCH_TERNARY:
            ternary = 1
        elif typ == p4_match_type.P4_MATCH_LPM:
            lpm += 1

        # Variable width field in table match key is not supported
        if not is_vwf(field):
            key_length += field.width
    if ternary or lpm > 1:
        table_type = "LOOKUP_TERNARY"
    elif lpm:
        table_type = "LOOKUP_LPM"
    else:
        table_type = "LOOKUP_EXACT"
    return (table_type, (key_length + 7) / 8)


# CAUTION: the result array contains the bytes in big endian order!
def int_to_big_endian_byte_array(val):
    """
    :param val: int
    :rtype:     (int, [int])
    """
    nbytes = 0
    res = []
    while val > 0:
        nbytes += 1
        res.append(int(val % 256))
        val /= 256
    res.reverse()
    return nbytes, res


def int_to_big_endian_byte_array_with_length(value, width):
    """CAUTION: the result array contains the bytes in big endian order!"""
    value_len, l = int_to_big_endian_byte_array(value)
    result = [0 for i in range(width - value_len)] + l[value_len - min(value_len, width): value_len]
    return result


def is_vwf(f):
    """Returns if the given field instance is a variable width field"""
    return f.width == p4.P4_AUTO_WIDTH


def field_max_width(f):
    """For normal fields returns their width, for variable width fields returns the maximum possible width in bits"""
    if not is_vwf(f): return f.width
    return (f.instance.header_type.max_length * 8) - sum([field[1] if field[1] != p4.P4_AUTO_WIDTH else 0 for field in f.instance.header_type.layout.items()])


def field_max_width_16(f):
    """For normal fields returns their width, for variable width fields returns the maximum possible width in bits"""
    if not f.field_ref.is_vw:
        return f.field_ref.size

    # TODO this code is untested yet
    nonvw_fields_width  = [fld.size for fld in dst.expr.header_ref.type.fields if not fld.is_vw]
    max_header_type_len = 12345 # TODO
    return max_header_type_len - nonvw_fields_width


def is_field_byte_aligned(f):
    """Returns if the given field is byte-aligned"""
    return field_max_width(f) % 8 == 0 and f.offset % 8 == 0;
