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

type_env = {}

def format_type_16(t, resolve_names = True):
    if t.node_type == 'Type_Void':
        return 'void'
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
    elif t.node_type == 'Type_Var':
        global type_env

        if t.name in type_env:
            return type_env[t.name]
        addWarning('using a named type parameter', 'no type found in environment for variable {}, defaulting to int'.format(t.name))
        return 'int /*type param {}*/'.format(t.name)
    elif t.node_type in {'Type_Enum', 'Type_Error'}:
        return 'enum ' + t.c_name
    elif t.node_type in {'Type_Extern', 'Type_Struct'}:
        return t.name
    else:
        addError('formatting type', 'The type %s is not supported yet!' % t.node_type)
        return ''

def pp_type_16(t): # Pretty print P4_16 type
    if t.node_type == 'Type_Boolean':
        return 'bool'
    elif t.node_type == 'Type_Bits':
        return ('int' if t.isSigned else 'bit') + '<' + str(t.size) + '>'
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
            return '%s %s;' % (format_type_16(d.type, False), d.name)
    elif d.node_type == 'Declaration_Instance':
        return 'struct {0} {1};\n{0}_init(&{1});'.format(format_type_16(d.type, False), d.name)
    elif d.node_type == 'P4Table' or d.node_type == 'P4Action':
        return ''
        #for m in d.type.type_ref.methods:
        #    if m.name != d.type.path.name:
        #        #[ ${d.name}.${m.name} = &${format_type_16(d.type)}_${m.name};
    else:
        addError('formatting declaration', 'Declaration of type %s is not supported yet!' % d.node_type)
        return ''

################################################################################

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
        if hasattr(s.left, 'field_ref'):
            # fldid(s.left.expr.header_ref, s.left.field_ref)
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
    statement_buffer_ret = statement_buffer
    statement_buffer = ""
    return statement_buffer_ret + ret

################################################################################

def resolve_reference(e):
    if hasattr(e, 'field_ref'):
        h = e.expr.header_ref
        f = e.field_ref
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


# A set of expression IDs that have already been generated.
generated_exprs = set()

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
            s += 'memcpy(buffer%s + (%s+7)/8, field_desc(pd, %s).byte_addr, (%s+7)/8);\n' % (expr.id, o, fldid(h, fs[0]), w)
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
    if e.node_type == 'Parameter':
        return format_type_16(e.type) + " " + e.name
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
        return format_expr_16(e.typeName);

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
        if e.id not in generated_exprs:
            prepend_statement(listexpression_to_buf(e))
            generated_exprs.add(e.id)
        return '(struct uint8_buffer_t) {{ .buffer =  buffer{}, .buffer_size = buffer{}_size }}'.format(e.id, e.id)
        # return 'buffer{}, buffer{}_size'.format(e.id, e.id)
    if e.node_type == 'SelectExpression':
        #Generate local variables for select values
        for k in e.select.components:
            if k.type.node_type == 'Type_Bits' and k.type.size <= 32:
                prepend_statement('{} {} = {};'.format(format_type_16(k.type), gen_var_name(k), format_expr_16(k)))
            elif k.type.node_type == 'Type_Bits' and k.type.size % 8 == 0:
                prepend_statement('uint8_t {0}[{1}];\n EXTRACT_BYTEBUF_PACKET(pd, {2}, {0});'
                                  .format(gen_var_name(k), k.type.size/8, format_expr_16(k, False)))
            else:
                addError('formatting select expression', 'Select on type %s is not supported!' % pp_type_16(k.type))

        cases = []
        for case in e.selectCases:
            cases_tmp = case.keyset.components if case.keyset.node_type == 'ListExpression' else [case.keyset]
            conds = []
            for k, c in zip(e.select.components, cases_tmp):
                select_type = k.type.node_type
                size = k.type.size #if k.type.node_type == 'Type_Bits' else 0
                case_type = c.node_type

                if case_type == 'DefaultExpression':
                    conds.append('true /* default */')
                elif case_type == 'Constant' and select_type == 'Type_Bits' and 32 < size and size % 8 == 0:
                    from  utils.hlir import int_to_big_endian_byte_array_with_length
                    l = int_to_big_endian_byte_array_with_length(c.value, size/8)
                    prepend_statement('uint8_t {0}[{1}] = {{{2}}};'.format(gen_var_name(c), size/8, ','.join([str(x) for x in l ])))
                    conds.append('memcmp({}, {}, {}) == 0'.format(gen_var_name(k), gen_var_name(c), size/8))
                elif size <= 32:
                    if case_type == 'Range':
                        conds.append('{0} <= {1} && {1} <= {2}'.format(format_expr_16(c.left), gen_var_name(k), format_expr_16(c.right)))
                    elif case_type == 'Mask':
                        conds.append('{0} & {1} == {2} & {1}'.format(format_expr_16(c.left), format_expr_16(c.right), gen_var_name(k)))
                    else:
                        if case_type not in {'Constant'}: #Trusted expressions
                            addWarning('formatting a select case', 'Select statement cases of type %s on %s might not work properly.'
                                       % (case_type, pp_type_16(k.type)))
                        conds.append('{} == {}'.format(gen_var_name(k), format_expr_16(c)))
                else:
                    addError('formatting a select case', 'Select statement cases of type %s on %s is not supported!'
                             % (case_type, pp_type_16(k.type)))
            cases.append('if({0}){{parser_state_{1}(pd, buf, tables);}}'.format(' && '.join(conds), format_expr_16(case.state)))
        return '\nelse\n'.join(cases)

    #These used to be PathExpressions
    if e.node_type in {'Declaration_Variable', 'Declaration_Instance', 'Method', 'P4Table', 'ParserState'}:
        return e.name

    if e.node_type == 'Member':
        if hasattr(e, 'field_ref'):
            if format_as_value == False:
                return fldid2(e.expr.header_ref, e.field_ref)
            else:
                return '(GET_INT32_AUTO_PACKET(pd, ' + e.expr.header_ref.id + ', ' + e.field_ref.id + '))'
        elif hasattr(e, 'header_ref'):
            return e.header_ref.id
        elif e.expr.node_type == 'Declaration_Variable':
            var = e.expr.name
            if e.expr.type.node_type == 'Type_Header':
                h = e.expr.type
                return '(GET_INT32_AUTO_BUFFER(' + var + ',' + var + '_var, field_' + h.name + "_" + e.member + '))'
            else:
                return format_expr_16(e.expr) + '.' + e.member
        else:
            if e.type.node_type in {'Type_Enum', 'Type_Error'}:
                return e.type.members.get(e.member).c_name
            return format_expr_16(e.expr) + '.' + e.member
    # TODO some of these are formatted as statements, we shall fix this
    if e.node_type == 'MethodCallExpression':
        if e.method.node_type == 'Member' and e.method.member == 'setValid':
            h = e.method.expr.header_ref
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
            if hasattr(e.method.expr, 'header_ref'):
                return "(pd->headers[%s].pointer != NULL)" % e.method.expr.header_ref.id
            else:
                return "(pd->headers[%s].pointer != NULL)" % format_expr_16(e.method.expr)
        elif e.arguments.is_vec() and e.arguments.vec != []:# and e.arguments[0].node_type == 'ListExpression':
            args = ", ".join([format_expr_16(arg) for arg in e.arguments])
            return format_expr_16(e.method) + '(' + args + ', pd, tables)'
#        elif e.arguments.is_vec() and e.arguments.vec != []:
#            addWarning("formatting an expression", "MethodCallExpression with arguments is not properly implemented yet.")
        else:
            return format_expr_16(e.method) + '(pd, tables)'
    addError("formatting an expression", "Expression of type %s is not supported yet!" % e.node_type)
    return ""

################################################################################

def gen_var_name(item):
    return 'value_%d' % item.id
