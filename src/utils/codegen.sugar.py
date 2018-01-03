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

from utils.misc import addWarning, addError

################################################################################

type_env = {}

def gen_format_type_16(t, resolve_names = True):
    if t.node_type == 'Type_Void':
        #[ void
    elif t.node_type == 'Type_Boolean':
        #[ bool
    elif t.node_type == 'Type_Bits':
        res = 'int' if t.isSigned else 'uint'
        if t.size <= 8:
            res += '8_t'
        elif t.size <= 16:
            res += '16_t'
        elif t.size <= 32:
            res += '32_t'
        else:
            name = "bit" if t.isSigned else "int"
            
            addError('formatting type', '{}<{}> too long, width only supported up to 32 bits'.format(name, t.size))
        return res
    elif t.node_type == 'Type_Name':
        if t.type_ref.node_type in {'Type_Enum', 'Type_Error'}:
            #[ enum ${t.type_ref.c_name}
        else:
            if not resolve_names:
                return t.type_ref.name

            global type_env

            if t.type_ref.name in type_env:
                return type_env[t.type_ref.name]
            addWarning('using a named type parameter', 'no type found in environment for variable {}, defaulting to int'.format(t.type_ref.name))
            #[ int /*type param ${t.type_ref.name}*/
    elif t.node_type in {'Type_Extern', 'Type_Struct'}:
        #[ ${t.name}
    else:
        addError('formatting type', 'The type %s is not supported yet!' % t.node_type)

def pp_type_16(t):
    """Pretty print P4_16 type"""
    if t.node_type == 'Type_Boolean':
        return 'bool'
    elif t.node_type == 'Type_Bits':
        return ('int' if t.isSigned else 'bit') + '<' + str(t.size) + '>'
    else:
        return str(t)

def gen_format_type_mask(t):
    if t.node_type == 'Type_Bits' and not t.isSigned:
        mask = hex((2 ** t.size) - 1)
        #[ $mask&
    else:
        addError('formatting a type mask', 'Currently only bit<w> is supported!')

def gen_format_method_parameters(parameters, method_type):
    res_params = []
    for (par,tpar) in zip(parameters, method_type.parameters.parameters):
        if hasattr(par, 'field_ref'):
            res_params.append('handle(header_desc_ins(pd, {}), {})'.format(par.expr.header_ref.id, par.field_ref.id))
        else:
            res_params.append(format_expr_16(par))
    #[ ${', '.join(res_params)}

def gen_format_declaration_16(d):
    if d.node_type == 'Declaration_Variable':
        if d.type.type_ref.node_type == 'Type_Header':
            #[ uint8_t ${d.name}[${d.type.type_ref.byte_width}];
            #[ uint8_t ${d.name}_var = 0;/* Width of the variable width field*/
        else:
            t = gen_format_type_16(d.type, False)
            #[ $t ${d.name};
    elif d.node_type == 'Declaration_Instance':
        t = gen_format_type_16(d.type, False)
        #[ struct $t ${d.name};
        #[ ${t}_init(&${d.name});
    elif d.node_type == 'P4Table' or d.node_type == 'P4Action':
        #[ /* nothing */

        #for m in d.type.type_ref.methods:
        #    if m.name != d.type.path.name:
        #        #[ ${d.name}.${m.name} = &${gen_format_type_16(d.type)}_${m.name};
    else:
        addError('formatting declaration', 'Declaration of type %s is not supported yet!' % d.node_type)

################################################################################

def is_metadata(e):
    if e.node_type == 'Member':
        if hasattr(e.expr, 'header_ref'):
            return e.expr.header_ref.type.type_ref.is_metadata
        elif hasattr(e.expr.type, 'is_metadata'):
            return e.expr.type.is_metadata
        else:
            return False
    return False


def is_std_metadata(e):
    return is_metadata(e) and e.expr.type.name == 'standard_metadata_t'

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

def int_to_big_endian_byte_array_with_length(value, width):
    array = []
    while value > 0:
        array.append(int(value % 256))
        value /= 256
    array.reverse()
    array_len = len(array)
    padded_array = [0 for i in range(width-array_len)] + array[array_len-min(array_len, width) : array_len]
    return '{' + ', '.join([str(x) for x in padded_array]) + '}'


def bit_bounding_unit(t):
    """The bit width of the smallest int that can contain the type,
    or the string "bytebuf" if it is larger than all possible container types."""
    if t.size <= 8:
        return 8
    if t.size <= 16:
        return 16
    if t.size <= 32:
        return 32
    return "bytebuf"

# TODO add all the special cases from the previous content of actions.c.py
def gen_format_statement_16(stmt):
    if stmt.node_type == 'AssignmentStatement':
        dst = stmt.left
        src = stmt.right

        if hasattr(dst, 'field_ref'):
            def member_to_field_id(member):
                return 'field_{}_{}'.format(member.expr.type.name, member.member)
            #TODO: handle preparsed fields, width assignment for vw fields and assignment to buffer instead header fields
            dst_width = dst.type.size
            dst_is_vw = dst.type.node_type == 'Type_Varbits'
            dst_bytewidth = (dst_width+7)/8

            assert(dst_width == src.type.size)
            assert(dst_is_vw == (src.type.node_type == 'Type_Varbits'))

            dst_header_id = 'header_instance_{}'.format(dst.expr.member if dst.expr.node_type == 'Member' else dst.expr.header_ref.name)
            dst_field_id = member_to_field_id(dst)

            if dst_width < 32:
                src_buffer = 'value32'
                if src.node_type == 'Member':
                    #[ $src_buffer = ${format_expr_16(src)};
                elif src.node_type == 'PathExpression':
                    #[ memcpy(&$src_buffer, parameters.${src.ref.name}, $dst_bytewidth);
                else:
                    #[ $src_buffer = ${format_expr_16(src)};

                #[ MODIFY_INT32_INT32_AUTO_PACKET(pd, $dst_header_id, $dst_field_id, $src_buffer)
            else:
                if src.node_type == 'Member':
                    src_pointer = 'value_{}'.format(src.id)
                    #[ uint8_t $src_pointer[$dst_bytewidth];

                    if hasattr(src, 'field_ref'):
                        #[ EXTRACT_BYTEBUF_PACKET(pd, header_instance_${src.expr.member}, ${member_to_field_id(src)}, ${src_pointer})
                        if dst_is_vw:
                            src_vw_bitwidth = 'pd->headers[header_instance_{}].var_width_field_bitwidth'.format(src.expr.member)
                            dst_bytewidth = '({}/8)'.format(src_vw_bitwidth)
                    else:
                        src_extract_params = '{0}, {0}_var, {1}, {2}'.format(src.expr.ref.name, member_to_field_id(src), src_pointer)
                        #[ EXTRACT_BYTEBUF_BUFFER($src_extract_params)
                        if dst_is_vw:
                            src_vw_bitwidth = '{}_var'.format(src.expr.ref.name)
                            dst_bytewidth = '({}/8)'.format(src_vw_bitwidth)
                elif src.node_type == 'PathExpression':
                    src_pointer = 'parameters.{}'.format(src.ref.name)
                elif src.node_type == 'Constant':
                    src_pointer = 'value_{}'.format(src.id)
                    #[ uint8_t $src_pointer[$dst_bytewidth] = ${int_to_big_endian_byte_array_with_length(src.value, dst_bytewidth)};
                else:
                    src_pointer = 'NOT_SUPPORTED'
                    addError('formatting statement', 'Unhandled right hand side in assignment statement: {}'.format(src))

                if dst_is_vw:
                    dst_fixed_size = dst.expr.header_ref.type.type_ref.bit_width - dst.field_ref.size
                    #[ pd->headers[$dst_header_id].length = ($dst_fixed_size + $src_vw_bitwidth)/8;
                    #[ pd->headers[$dst_header_id].var_width_field_bitwidth = $src_vw_bitwidth;

                #[ MODIFY_BYTEBUF_BYTEBUF_PACKET(pd, $dst_header_id, $dst_field_id, $src_pointer, $dst_bytewidth)
        else:
            #[ ${format_expr_16(dst)} = ${format_expr_16(src)};

    elif stmt.node_type == 'BlockStatement':
        for c in stmt.components:
            #= format_statement_16(c)
    elif stmt.node_type == 'IfStatement':
        t = format_statement_16(stmt.ifTrue) if hasattr(stmt, 'ifTrue') else ';'
        f = format_statement_16(stmt.ifFalse) if hasattr(stmt, 'ifFalse') else ';'
        cond = format_expr_16(stmt.condition)
        #[ if( $cond ) {
        #[ $t
        #[ } else {
        #[ $f
        #[ }
    elif stmt.node_type == 'MethodCallStatement':
        m = stmt.methodCall.method

        if m.node_type == 'Method' and m.name == 'digest':
            digest_name = stmt.methodCall.typeArguments[0].name
            port, fields = stmt.methodCall.arguments

            # TODO this will probably have to be generalised later on
            #[ struct type_field_list fields;
            #[ fields.fields_quantity = ${len(fields)};
            #[ fields.field_offsets = malloc(sizeof(uint8_t*)*fields.fields_quantity);
            #[ fields.field_widths = malloc(sizeof(uint8_t*)*fields.fields_quantity);

            for idx, f in enumerate(fields.components):
                if f.expr.type.is_metadata:
                    #[ fields.field_offsets[$idx] = (uint8_t*) field_desc(pd, field_instance_${f.expr.name}_${f.member}).byte_addr;
                    #[ fields.field_widths[$idx]  =            field_desc(pd, field_instance_${f.expr.name}_${f.member}).bitwidth;
                else:
                    #[ fields.field_offsets[$idx] = (uint8_t*) field_desc(pd, field_instance_${f.expr.member}_${f.field_ref.name}).byte_addr;
                    #[ fields.field_widths[$idx]  =            field_desc(pd, field_instance_${f.expr.member}_${f.field_ref.name}).bitwidth;
            #[ generate_digest(bg,"${digest_name}",0,&fields);
            #[ sleep(1);
        else:
            mcall = format_expr_16(stmt.methodCall)
            if mcall:
                #[ $mcall;
            else:
                addError('generating method call statement', 'Invalid method call {}'.format(stmt.methodCall))
                #[ /* unhandled method call ${stmt.methodCall} */


################################################################################

def resolve_reference(e):
    if hasattr(e, 'field_ref'):
        h = e.expr.header_ref
        f = e.field_ref
        return (h, f)
    else:
        return e

def is_subsequent((h1, f1), (h2, f2)):
    fs = h1.type.type_ref.fields.vec
    return h1 == h2 and fs.index(f1) + 1 == fs.index(f2)

def groupby(xs, fun):
    """Groups the elements of a list.
    The upcoming element will be grouped if
    fun(last element of the group, upcoming) evaluates to true."""
    if not xs:
        yield []
        return

    elems = []
    for x in xs:
        if elems == []:
            elems = [x]
        elif not fun(elems[-1], x):
            yield elems
            elems = [x]
        else:
            elems.append(x)

    if elems != []:
        yield elems

def group_references(refs):
    for xs in groupby(refs, lambda x1, x2: isinstance(x1, tuple) and isinstance(x2, tuple) and is_subsequent(x1, x2)):
        yield (xs[0][0], map(lambda (hdr, fld): fld, xs))

def fldid(h, f):
    if h.node_type == 'PathExpression':
        return 'field_instance_' + h.ref.name + '_' + f.name
    else:
        return 'field_instance_' + h.name + '_' + f.name
def fldid2(h, f): return h.id + ',' +  f.id


# A set of expression IDs that have already been generated.
generated_exprs = set()

def convert_component(component):
    if component.node_type == 'Member':
        hdr      = component.expr
        fld_name = component.member
        fld      = hdr.type.fields.get(fld_name)
        return (hdr, fld)

    addError('generating list expression buffer', 'List element (%s) not supported!' % component)
    return None

def listexpression_to_buf(expr):
    def width(hdr, fld):
        if fld.is_vw: return 'field_desc(pd, %s).bitwidth'%fldid(hdr, fld)
        return str(fld.size)

    s = ""
    o = '0'
    components = [c if type(c) == tuple else convert_component(c) for c in map(resolve_reference, expr.components)]
    for h, fs in group_references(components):
        w = '+'.join(map(lambda f: width(h, f), fs))
        s += 'memcpy(buffer%s + (%s+7)/8, field_desc(pd, %s).byte_addr, (%s+7)/8);\n' % (expr.id, o, fldid(h, fs[0]), w)
        o += '+'+w
    return 'int buffer{0}_size = ({1}+7)/8;\nuint8_t buffer{0}[buffer{0}_size];\n'.format(expr.id, o) + s

################################################################################

def gen_format_expr_16(e, format_as_value=True):
    simple_binary_ops = {'Div':'/', 'Mod':'%',                                 #Binary arithmetic operators
                         'Grt':'>', 'Geq':'>=', 'Lss':'<', 'Leq':'<=',         #Binary comparison operators
                         'BAnd':'&', 'BOr':'|', 'BXor':'^',                    #Bitwise operators
                         'LAnd':'&&', 'LOr':'||',                              #Boolean operators
                         'Equ':'==', 'Neq':'!='}                               #Equality operators

    complex_binary_ops = {'Add':'+', 'Sub':'-', 'Mul':'*', 'Shl':'<<', 'Shr':'>>'}

    if e is None:
        return "FORMAT_EXPR_16(None)"
    elif e.node_type == 'DefaultExpression':
        return ""
    elif e.node_type == 'Parameter':
        return format_type_16(e.type) + " " + e.name
    elif e.node_type == 'Constant':
        if e.type.node_type == 'Type_Bits':
            # 4294967136 versus (uint32_t)4294967136
            return '(' + format_type_16(e.type) + ')' + str(e.value)
        else:
            return str(e.value)
    elif e.node_type == 'BoolLiteral':
        return 'true' if e.value else 'false'
    elif e.node_type == 'StringLiteral':
        return '"' + e.value + '"';
    elif e.node_type == 'TypeNameExpression':
        return format_expr_16(e.typeName.type_ref);

    elif e.node_type == 'Neg':
        if e.type.node_type == 'Type_Bits' and not e.type.isSigned:
            return '(' + format_type_mask(e.type) + '(' + str(2**e.type.size) + '-' + format_expr_16(e.expr) + '))'
        else:
            return '(-' + format_expr_16(e.expr) + ')'
    elif e.node_type == 'Cmpl':
        return '(' + format_type_mask(e.type) + '(~' + format_expr_16(e.expr) + '))'
    elif e.node_type == 'LNot':
        return '(!' + format_expr_16(e.expr) + ')'

    elif e.node_type in simple_binary_ops:
        return '(' + format_expr_16(e.left) + simple_binary_ops[e.node_type] + format_expr_16(e.right) + ')'

    #Subtraction on unsigned values is performed by adding the negation of the second operand
    elif e.node_type == 'Sub' and e.type.node_type == 'Type_Bits' and not e.type.isSigned:
        return '(' + format_type_mask(e.type) + '(' + format_expr_16(e.left) + '+(' + str(2**e.type.size) + '-' + format_expr_16(e.right) + ')))'
    #Right shift on signed values is performed with a shift width check
    elif e.node_type == 'Shr' and e.type.node_type == 'Type_Bits' and e.type.isSigned:
        return '(({1}>{2}) ? 0 : ({0} >> {1}))'.format(format_expr_16(e.left), format_expr_16(e.right), e.type.size)
    #These formatting rules MUST follow the previous special cases
    elif e.node_type in complex_binary_ops:
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

    elif e.node_type == 'Mux':
        return '(' + format_expr_16(e.e0) + '?' + format_expr_16(e.e1) + ':' + format_expr_16(e.e2) + ')'

    elif e.node_type == 'Slice':
        return '(' + format_type_mask(e.type) + '(' + format_expr_16(e.e0) + '>>' + format_expr_16(e.e2) + '))'

    elif e.node_type == 'Concat':
        return '((' + format_expr_16(e.left) + '<<' + str(e.right.type.size) + ') | ' + format_expr_16(e.right) + ')'

    elif e.node_type == 'Cast':
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

    elif e.node_type == 'ListExpression':
        if e.id not in generated_exprs:
            prepend_statement(listexpression_to_buf(e))
            generated_exprs.add(e.id)
        return '(struct uint8_buffer_t) {{ .buffer =  buffer{}, .buffer_size = buffer{}_size }}'.format(e.id, e.id)
        # return 'buffer{}, buffer{}_size'.format(e.id, e.id)
    elif e.node_type == 'SelectExpression':
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
                    byte_array = int_to_big_endian_byte_array_with_length(c.value, size/8)
                    prepend_statement('uint8_t {}[{}] = {};'.format(gen_var_name(c), size/8, byte_array))
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

    elif e.node_type == 'PathExpression':
        return e.ref.name

    elif e.node_type == 'Member':
        if hasattr(e, 'field_ref'):
            if format_as_value == False:
                return fldid2(e.expr.header_ref, e.field_ref)
            else:
                return '(GET_INT32_AUTO_PACKET(pd, ' + e.expr.header_ref.id + ', ' + e.field_ref.id + '))'
        elif hasattr(e, 'header_ref'):
            return e.header_ref.id
        elif e.expr.node_type == 'PathExpression':
            var = e.expr.ref.name
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
    elif e.node_type == 'MethodCallExpression':
        if e.method.node_type == 'Member' and e.method.member == 'setValid':
            h = e.method.expr.header_ref
            # TODO is this the max size?
            length = (sum([f.size if not f.is_vw else 0 for f in h.type.type_ref.fields])+7)/8

            #[ pd->headers[${h.id}] = (header_descriptor_t) {
            #[     .type = ${h.id},
            #[     .length = $length,
            #[     .pointer = calloc(${h.type.type_ref.byte_width}, sizeof(uint8_t)),
            #[     /*TODO determine and set this field*/
            #[     .var_width_field_bitwidth = 0,
            #[ };
        elif e.method.node_type == 'Member' and e.method.member == 'emit':
            arg0 = e.arguments[0].member
            haddr = "pd->headers[header_instance_%s].pointer"%arg0
            hlen  = "pd->headers[header_instance_%s].length" %arg0

            #[ if (emit_addr != $haddr) {
            #[     memcpy(emit_addr, $haddr, $hlen);
            #[ }
            #[ emit_addr += $hlen;
        elif e.method.node_type == 'Member' and e.method.member == 'isValid':
            if hasattr(e.method.expr, 'header_ref'):
                return "(pd->headers[%s].pointer != NULL)" % e.method.expr.header_ref.id
            else:
                return "(pd->headers[%s].pointer != NULL)" % format_expr_16(e.method.expr)
        elif e.arguments.is_vec() and e.arguments.vec != []:# and e.arguments[0].node_type == 'ListExpression':
            return '{}({}, pd, tables)'.format(e.method.ref.name, format_method_parameters(e.arguments, e.method.ref.type))
       # elif e.arguments.is_vec() and e.arguments.vec != []:
       #     addWarning("formatting an expression", "MethodCallExpression with arguments is not properly implemented yet.")
        else:
            return format_expr_16(e.method) + '(pd, tables)'
    else:
        addError("formatting an expression", "Expression of type %s is not supported yet!" % e.node_type)

################################################################################

def format_declaration_16(d):
    global file_sugar_style
    with SugarStyle("no_comment"):
        return gen_format_declaration_16(d)

def format_type_16(t, resolve_names = True):
    global file_sugar_style
    with SugarStyle("inline_comment"):
        return gen_format_type_16(t, resolve_names)

def format_method_parameters(ps, mt):
    global file_sugar_style
    with SugarStyle("inline_comment"):
        return gen_format_method_parameters(ps, mt)

def format_expr_16(e, format_as_value=True):
    global file_sugar_style
    with SugarStyle("inline_comment"):
        return gen_format_expr_16(e, format_as_value)

def format_statement_16(stmt):
    global statement_buffer
    statement_buffer = ""

    ret = gen_format_statement_16(stmt)

    statement_buffer_ret = statement_buffer
    statement_buffer = ""
    return statement_buffer_ret + ret


def format_type_mask(t):
    global file_sugar_style
    with SugarStyle("inline_comment"):
        return gen_format_type_mask(t)

def gen_var_name(item):
    #[ value_${item.id}
