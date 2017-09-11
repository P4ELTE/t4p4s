# Copyright 2017 Eotvos Lorand University, Budapest, Hungary
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

from misc import addWarning

################################################################################
from misc import addWarning, addError

def format_type_16(t):
    if t.node_type == 'Type_Boolean':
        return 'bool'
    if t.node_type == 'Type_Bits':
        res = 'int'
        if t.size % 8 == 0 and t.size <= 32:
            res = res + str(t.size) + '_t'
        if not t.isSigned:
            res = 'u' + res
        return res
    if t.node_type == 'Type_Name':
        return format_expr_16(t.path)

def format_declaration_16(d):
    if d.node_type == 'Declaration_Variable':
        return '%s %s;' % (format_type_16(d.type), d.name)
    if d.node_type == 'Declaration_Instance':
        return 'struct {0} {1};\n{0}_init(&{1});'.format(format_type_16(d.type), d.name)
    if d.node_type == 'P4Table' or d.node_type == 'P4Action':
        return ''
#        for m in d.type.ref.methods:
#            if m.name != d.type.path.name:
#                #[ ${d.name}.${m.name} = &${format_type_16(d.type)}_${m.name};

################################################################################

def has_field_ref(node):
    return hasattr(node, 'ref') and node.ref.node_type == 'StructField' and not hasattr(node.ref, 'header_type')

def has_header_ref(node):
    return hasattr(node, 'ref') and node.ref.node_type == 'StructField' and hasattr(node.ref, 'header_type')

statement_buffer = ""

def prepend_statement(s):
    global statement_buffer
    statement_buffer += "\n" + s

def statement_buffer_value():
    global statement_buffer
    ret = statement_buffer
    statement_buffer = ""
    return ret

################################################################################

def format_statement_16(s):
    ret = ""
    global statement_buffer
    statement_buffer = ""
    if s.node_type == 'AssignmentStatement':
        if has_field_ref(s.left):
            # fldid(s.left.expr.ref, s.left.ref)
            ret += 'MODIFY_INT32_INT32_AUTO_PACKET(pd, %s, %s);' % (format_expr_16(s.left, False), format_expr_16(s.right)) # TODO
        else:
            ret += '%s = %s;' % (format_expr_16(s.left), format_expr_16(s.right))
    if s.node_type == 'BlockStatement':
        for c in s.components:
            ret += format_statement_16(c)
    if s.node_type == 'IfStatement':
        t = format_statement_16(s.ifTrue) if hasattr(s, 'ifTrue') else ';'
        f = format_statement_16(s.ifFalse) if hasattr(s, 'ifFalse') else ';'
        ret += 'if( %s ) { %s } else { %s }' % (format_expr_16(s.condition), t, f)
    if s.node_type == 'MethodCallStatement':
        if format_expr_16(s.methodCall) is None:
            print (s.methodCall)
            ret += ':('
        else:
            ret += format_expr_16(s.methodCall) + ';'
    return statement_buffer + ret

################################################################################

def resolve_reference(e):
    if has_field_ref(e):
        h = e.expr.ref
        f = e.ref
        return (h, f)
    else:
        return e

def subsequent_fields((h1, f1), (h2, f2)):
    fs = h1.type.fields.vec
    return h1 == h2 and fs.index(f1) + 1 == fs.index(f2)

def group_references(refs):
    ret = []
    i = 0
    while i < len(refs):
        if not isinstance(refs[i], tuple):
            ret.append(refs[i])
        else:
            h, f = refs[i]
            fs = [f]
            j = 1
            while i+j < len(refs) and subsequent_fields((h, fs[-1]), refs[i+j]):
                fs.append(refs[i+j][1])
                j += 1
            i += j
            ret.append((h, fs))
    return ret

def fldid(h, f): return 'field_instance_' + h.name + '_' + f.name
def fldid2(h, f): return h.id + ',' +  f.id

def listexpression_to_buf(expr):
    s = ""
    o = '0'
    for x in group_references(map(resolve_reference, expr.components)):
        if isinstance(x, tuple):
            h, fs = x
            def width(f):
                if f.is_vw: return 'field_desc(pd, %s).bitwidth'%fldid(h, f)
                else: return str(f.size)
            w = '+'.join(map(width, fs))
            s += 'memcpy(buffer%s + (%s+7)/8, field_desc(pd, %s).byte_addr, (%s+7)/8);' % (expr.id, o, fldid(h, fs[0]), w)
            o += '+'+w
    return 'int buffer{0}_size = ({1}+7)/8;\nuint8_t* buffer{0} = calloc(buffer{0}_size, sizeof(uint8_t));\n'.format(expr.id, o) + s

################################################################################

def format_expr_16(e, format_as_value=True):
    if e is None:
        return "FORMAT_EXPR_16(None)"
    if e.node_type == 'DefaultExpression':
        return ""
    if e.node_type == 'Constant':
        if e.type.node_type == 'Type_Bits':
            # 4294967136 versus (uint32_t)4294967136
            return '(' + format_type_16(e.type) + ')' + str(e.value)
        else:
            return str(e.value)
    if e.node_type == 'BoolLiteral':
        return 'true' if e.value else 'false'
    if e.node_type == 'StringLiteral':
        return '"' + e.value + '"';

    simple_unary_ops = {'Neg':'-', 'Cmpl':'~', 'LNot':'!'}
    if e.node_type in simple_unary_ops:
        return simple_unary_ops[e.node_type] + '(' + format_expr_16(e.expr) + ')'

    simple_binary_ops = {'Add':'+', 'Sub':'-', 'Mul':'*', 'Div':'/', 'Mod':'%',#Binary arithmetic operators
                         'Grt':'>', 'Geq':'>=', 'Lss':'<', 'Leq':'<=',         #Binary comparison operators
                         'BAnd':'&', 'BOr':'|', 'BXor':'^',                    #Bitwise operators
                         'LAnd':'&&', 'LOr':'||',                              #Boolean operators
                         'Shl':'<<', 'Shr':'>>',                               #Shift operators
                         'Equ':'==', 'Neq':'!='}                               #Equality operators
    if e.node_type in simple_binary_ops:
        return '(' + format_expr_16(e.left) + simple_binary_ops[e.node_type] + format_expr_16(e.right) + ')'

    if e.node_type == 'Mux':
        return '(' + format_expr_16(e.e0) + '?' + format_expr_16(e.e1) + ':' + format_expr_16(e.e2) + ')'

    if e.node_type == 'Cast':
        #TODO: add better cast for not byte-wide types
        return '((' + format_type_16(e.destType) + ')' + format_expr_16(e.expr) + ')'

    if e.node_type == 'ListExpression':
#        return ",".join(map(format_expr_16, e.components))
        prepend_statement(listexpression_to_buf(e))
        return 'buffer' + str(e.id) + ', ' + 'buffer' + str(e.id) + '_size'
    if e.node_type == 'SelectExpression':
        head = format_expr_16(e.select)
        headname = head.split(',')[0]
        cases = []
        for c in e.selectCases: # selectCase
            casebody = "parser_state_" + format_expr_16(c.state) + "(pd, buf, tables);"
            if c.keyset.node_type == 'DefaultExpression':
                cases.append(casebody + " // default")
            elif c.keyset.node_type == 'Constant':
                from  utils.hlir import int_to_big_endian_byte_array
                value_name = 'value' + str(c.keyset.id)
                value_len, l = int_to_big_endian_byte_array(c.keyset.value)
                prepend_statement('uint8_t ' + value_name + '[' + str(value_len) + '] = {' + ','.join([str(x) for x in l ]) + '};')
                cases.append("if ( memcmp(%s, %s, %s) == 0) { %s }" % (headname, value_name, value_len, casebody))
        return '\nelse\n'.join(cases)
    if e.node_type == 'PathExpression':
        return format_expr_16(e.path)
    if e.node_type == 'Path':
        if e.absolute:
            return "???" #TODO
        else:
            return e.name
    if e.node_type == 'Member':
        if has_field_ref(e) and hasattr(e.expr, 'ref') and format_as_value == False:
            return fldid2(e.expr.ref, e.ref)
        elif has_header_ref(e):
            return e.ref.id
        elif e.expr.node_type == 'PathExpression':
            var = e.expr.path.name
            if e.expr.type.node_type == 'Type_Header':
                h = e.expr.type
                return '(GET_INT32_AUTO_BUFFER(' + var + ',' + var + '_var, field_' + h.name + "_" + e.member + '))'
            else:
                return format_expr_16(e.expr) + '.' + e.member
        elif e.expr.node_type == 'Member':
            return '(GET_INT32_AUTO_PACKET(pd, ' + e.expr.ref.id + ', ' + e.ref.id + '))'
        else:
            return format_expr_16(e.expr) + '.' + e.member
    # TODO some of these are formatted as statements, we shall fix this
    if e.node_type == 'MethodCallExpression':
        if e.method.node_type == 'Member' and e.method.member == 'setValid':
            h = e.method.expr.ref
            ret  = 'pd->headers['+h.id+'] = (header_descriptor_t) {'
            ret += ' .type = '+h.id+','
#            ret += ' .length = header_info('+h.id+').bytewidth,
            ret += ' .length = %s,'%str((sum([f.size if not f.is_vw else 0 for f in h.type.fields])+7)/8)
            ret += ' .pointer = calloc(%s, sizeof(uint8_t)),'%str(h.type.byte_width) # TODO is this the max size?
            ret += ' .var_width_field_bitwidth = 0 };' # TODO determine and set this field
            return ret + " // hdr.*.setValid()"
        if e.method.node_type == 'Member' and e.method.member == 'emit':
            haddr = "pd->headers[header_instance_%s].pointer"%e.arguments[0].member
            hlen  = "pd->headers[header_instance_%s].length" %e.arguments[0].member
            ret = ""
            ret += "if(emit_addr != %s)" % haddr
            ret += "memcpy(emit_addr, %s, %s);" % (haddr, hlen)
            ret += "emit_addr += %s;" % hlen
            return ret
        elif e.method.node_type == 'Member' and e.method.member == 'isValid':
            if e.method.expr.get_attr('ref') is not None:
                return "pd->headers[%s].pointer != NULL" % e.method.expr.ref.id
            else:
                return "pd->headers[%s].pointer != NULL" % format_expr_16(e.method.expr)
        elif e.arguments.is_vec() and e.arguments.vec != []:# and e.arguments[0].node_type == 'ListExpression':
            return format_expr_16(e.method) + '(' + format_expr_16(e.arguments[0]) + ', pd, tables)'
#        elif e.arguments.is_vec() and e.arguments.vec != []:
#            addWarning("formatting an expression", "MethodCallExpression with arguments is not properly implemented yet.")
        else:
            return format_expr_16(e.method) + '(pd, tables)'
    addError("formatting an expression", "Expression of type %s is not supported yet!" % e.node_type)
    return ""
