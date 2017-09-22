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
    elif t.node_type == 'Type_Bits':
        res = 'int' if t.isSigned else 'uint'
        if t.size <= 8:
            res += '8_t'
        elif t.size <= 16:
            res += '16_t'
        elif t.size <= 32:
            res += '32_t'
        else:
            addError('formatting type', 'The maximum supported bitwidth for bit<w> and int<w> are 32 bits. (found %s)' % t.size)
        return res
    elif t.node_type == 'Type_Name':
        return format_expr_16(t.path)
    else:
        addError('formatting type', 'The type %s is not supported yet!' % t.node_type)
        return ''

def pp_type_16(t): # Pretty print P4_16 type
    if t.node_type == 'Type_Boolean':
        return 'bool'
    elif t.node_type == 'Type_Bits':
        return ('int' if t.isSigned else 'bit') + '<' + str(t.size) + '>'
    elif t.node_type == 'Type_Name':
        return format_expr_16(t.path)
    else:
        return str(t)

def format_type_mask(t):
    if t.node_type == 'Type_Bits' and not t.isSigned:
        return hex((2 ** t.size) - 1) + ' & '
    else:
        addError('formatting a type mask', 'Currently only bit<w> is supported!')
        return ''

def format_declaration_16(d):
    if d.node_type == 'Declaration_Variable':
        if d.type.node_type == 'Type_Header':
            return 'uint8_t {0}[{1}];\n uint8_t {0}_var = 0; // Width of the variable width field'.format(d.name, d.type.byte_width)
        else:
            return '%s %s;' % (format_type_16(d.type), d.name)
    elif d.node_type == 'Declaration_Instance':
        return 'struct {0} {1};\n{0}_init(&{1});'.format(format_type_16(d.type), d.name)
    elif d.node_type == 'P4Table' or d.node_type == 'P4Action':
        return ''
        #for m in d.type.ref.methods:
        #    if m.name != d.type.path.name:
        #        #[ ${d.name}.${m.name} = &${format_type_16(d.type)}_${m.name};
    else:
        addError('formatting declaration', 'Declaration of type %s is not supported yet!' % d.node_type)
        return ''

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
            i += 1
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
        else:
            addError('generating list expression buffer', 'List element (%s) not supported!' % x)
    return 'int buffer{0}_size = ({1}+7)/8;\nuint8_t buffer{0}[buffer{0}_size];\n'.format(expr.id, o) + s

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
    if e.node_type == 'TypeNameExpression':
        return format_expr_16(e.typeName.path);

    if e.node_type == 'Neg':
        if e.type.node_type == 'Type_Bits' and not e.type.isSigned:
            return '(' + format_type_mask(e.type) + '(' + str(2**e.type.size) + '-' + format_expr_16(e.expr) + '))'
        else:
            return '(-' + format_expr_16(e.expr) + ')'
    if e.node_type == 'Cmpl':
        return '(' + format_type_mask(e.type) + '(~' + format_expr_16(e.expr) + '))'
    if e.node_type == 'LNot':
        return '(!' + format_expr_16(e.expr) + ')'

    simple_binary_ops = {'Div':'/', 'Mod':'%',                                 #Binary arithmetic operators
                         'Grt':'>', 'Geq':'>=', 'Lss':'<', 'Leq':'<=',         #Binary comparison operators
                         'BAnd':'&', 'BOr':'|', 'BXor':'^',                    #Bitwise operators
                         'LAnd':'&&', 'LOr':'||',                              #Boolean operators
                         'Equ':'==', 'Neq':'!='}                               #Equality operators
    if e.node_type in simple_binary_ops:
        return '(' + format_expr_16(e.left) + simple_binary_ops[e.node_type] + format_expr_16(e.right) + ')'

    #Subtraction on unsigned values is performed by adding the negation of the second operand
    if e.node_type == 'Sub' and e.type.node_type == 'Type_Bits' and not e.type.isSigned:
        return '(' + format_type_mask(e.type) + '(' + format_expr_16(e.left) + '+(' + str(2**e.type.size) + '-' + format_expr_16(e.right) + ')))'
    #Right shift on signed values is performed with a shift width check
    if e.node_type == 'Shr' and e.type.node_type == 'Type_Bits' and e.type.isSigned:
        return '(({1}>{2}) ? 0 : ({0} >> {1}))'.format(format_expr_16(e.left), format_expr_16(e.right), e.type.size)
    #These formatting rules MUST follow the previous special cases
    complex_binary_ops = {'Add':'+', 'Sub':'-', 'Mul':'*', 'Shl':'<<', 'Shr':'>>'}
    if e.node_type in complex_binary_ops:
        temp_expr = '(' + format_expr_16(e.left) + complex_binary_ops[e.node_type] + format_expr_16(e.right) + ')'
        if e.type.node_type == 'Type_InfInt':
            return temp_expr
        elif e.type.node_type == 'Type_Bits':
            if not e.type.isSigned:
                return '(' + format_type_mask(e.type) + temp_expr + ')'
            else:
                if e.type.size in {8,16,32}:
                    return '((' + format_type_16(e.type) + ') ' + temp_expr + ')'
                else:
                    addError('formatting an expression', 'Expression of type %s is not supported on int<%s>. (Only int<8>, int<16> and int<32> are supported.)' % (e.node_type, e.type.size))
                    return ''

    if e.node_type == 'Mux':
        return '(' + format_expr_16(e.e0) + '?' + format_expr_16(e.e1) + ':' + format_expr_16(e.e2) + ')'

    if e.node_type == 'Slice':
        return '(' + format_type_mask(e.type) + '(' + format_expr_16(e.e0) + '>>' + format_expr_16(e.e2) + '))'

    if e.node_type == 'Concat':
        return '((' + format_expr_16(e.left) + '<<' + str(e.right.type.size) + ') | ' + format_expr_16(e.right) + ')'

    if e.node_type == 'Cast':
        if e.expr.type.node_type == 'Type_Bits' and not e.expr.type.isSigned and e.expr.type.size == 1 \
                and e.destType.node_type == 'Type_Boolean':        #Cast from bit<1> to bool
            return '(' + format_expr_16(e.expr) + ')'
        elif e.expr.type.node_type == 'Type_Boolean' and e.destType.node_type == 'Type_Bits' and not e.destType.isSigned \
                and e.destType.size == 1:                          #Cast from bool to bit<1>
            return '(' + format_expr_16(e.expr) + '? 1 : 0)'
        elif e.expr.type.node_type == 'Type_Bits' and e.destType.node_type == 'Type_Bits':
            if e.expr.type.isSigned == e.destType.isSigned:
                if not e.expr.type.isSigned:                       #Cast from bit<w> to bit<v>
                    if e.expr.type.size > e.destType.size:
                        return '(' + format_type_mask(e.destType) + format_expr_16(e.expr) + ')'
                    else:
                        return format_expr_16(e.expr)
                else:                                              #Cast from int<w> to int<v>
                    return '((' + format_type_16(e.destType) + ') ' + format_expr_16(e.expr) + ')'
            elif e.expr.type.isSigned and not e.destType.isSigned: #Cast from int<w> to bit<w>
                return '(' + format_type_mask(e.destType) + format_expr_16(e.expr) + ')'
            elif not e.expr.type.isSigned and e.destType.isSigned: #Cast from bit<w> to int<w>
                if e.destType.size in {8,16,32}:
                    return '((' + format_type_16(e.destType) + ')' + format_expr_16(e.expr) + ')'
                else:
                    addError('formatting an expression', 'Cast from bit<%s> to int<%s> is not supported! (Only int<8>, int<16> and int<32> are supported.)' % e.destType.size)
                    return ''
        #Cast from int to bit<w> and int<w> are performed by P4C
        addError('formatting an expression', 'Cast from %s to %s is not supported!' % (pp_type_16(e.expr.type), pp_type_16(e.destType)))
        return ''

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
        if has_field_ref(e) and hasattr(e.expr, 'ref'):
            if format_as_value == False:
                return fldid2(e.expr.ref, e.ref)
            else:
                return '(GET_INT32_AUTO_PACKET(pd, ' + e.expr.ref.id + ', ' + e.ref.id + '))'
        elif has_header_ref(e):
            return e.ref.id
        elif e.expr.node_type == 'PathExpression':
            var = e.expr.path.name
            if e.expr.type.node_type == 'Type_Header':
                h = e.expr.type
                return '(GET_INT32_AUTO_BUFFER(' + var + ',' + var + '_var, field_' + h.name + "_" + e.member + '))'
            else:
                return format_expr_16(e.expr) + '.' + e.member
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
                return "(pd->headers[%s].pointer != NULL)" % e.method.expr.ref.id
            else:
                return "(pd->headers[%s].pointer != NULL)" % format_expr_16(e.method.expr)
        elif e.arguments.is_vec() and e.arguments.vec != []:# and e.arguments[0].node_type == 'ListExpression':
            return format_expr_16(e.method) + '(' + format_expr_16(e.arguments[0]) + ', pd, tables)'
#        elif e.arguments.is_vec() and e.arguments.vec != []:
#            addWarning("formatting an expression", "MethodCallExpression with arguments is not properly implemented yet.")
        else:
            return format_expr_16(e.method) + '(pd, tables)'
    addError("formatting an expression", "Expression of type %s is not supported yet!" % e.node_type)
    return ""
