# SPDX-License-Identifier: Apache-2.0
# Copyright 2017 Eotvos Lorand University, Budapest, Hungary

from inspect import getmembers, isfunction
from enum import Enum
import sys

from itertools import takewhile

from hlir16.hlir_utils import align8_16_32
from hlir16.hlir_ops import elementwise_binary_ops, simple_binary_ops, complex_binary_ops
from hlir16.hlir_model import packets_by_model
from hlir16.p4node import P4Node

from compiler_log_warnings_errors import addWarning, addError
from compiler_common import types, with_base, resolve_reference, is_subsequent, groupby, group_references, fldid, fldid2, pp_type_16, make_const, SugarStyle, prepend_statement, append_statement, is_control_local_var, generate_var_name, pre_statement_buffer, post_statement_buffer, enclosing_control, unique_everseen, unspecified_value, get_raw_hdr_name, get_hdr_name, get_hdrfld_name, split_join_text
from utils.extern import get_smem_name

################################################################################

# note: some functions with well-known names in C have to be renamed
funname_map = {
    "random": "random_fun",
}

################################################################################

def sizeup(size, use_array):
    return 8 if use_array else 8 if size <= 8 else 16 if size <= 16 else 32 if size <= 32 else 8

def add_prefix(prefix, txt):
    if txt.startswith(prefix):
        return txt
    return f'{prefix}{txt}'


def type_to_str(t):
    if t.node_type == 'Type_Bits':
        sign = 'int' if t.isSigned else 'uint'
        return f'{sign}{t.size}_t'
    return f'{t.name}_t'

def append_type_postfix(name):
    return name if name.endswith('_t') else f'{name}_t'

def inf_int_c_type():
    ### Our C representation of the "indefinite length int" type.
    return 'int32_t'

def inf_int_short_c_type():
    ### The short name of the "indefinite length int" type.
    return 'i32'

def gen_format_type(t, resolve_names = True, use_array = False, addon = "", no_ptr = False, type_args = [], is_atomic = False, is_const = False):
    """Returns a type. If the type has a part that has to come after the variable name in a declaration,
    such as [20] in uint8_t varname[20], it should be separated with a space."""

    if t.node_type == 'Type_Header':
        #[ HDRT(${t.name}) $addon
    elif t.node_type == 'Type_Stack':
        #[ int /*TODO_Stack_t*/
    elif t.node_type == 'Type_Void':
        #[ void
    elif t.node_type == 'Type_InfInt':
        #[ ${inf_int_c_type()}
    elif t.node_type == 'Type_Boolean':
        #[ bool $addon
    elif t.node_type == 'Type_Error':
        name = t.c_name if 'c_name' in t else t.name
        #[ ${name}_t
    elif t.node_type == 'Type_List':
        #[ uint8_buffer_t
    elif t.node_type == 'Type_String':
        if is_const:
            #[ const char*
        else:
            #[ char*
    elif t.node_type == 'Type_Varbits':
        #[ uint8_t $addon[${(t.size+7)//8}] /* preliminary type for varbits */
    elif t.node_type == 'Type_Struct':
        base_name = re.sub(r'_t$', '', t.name)
        ptr = '*' if not no_ptr else ''
        #[ ${base_name}_t${ptr}
    elif t.node_type == 'Type_Var' and t.urtype.name in types.env():
        #[ ${types.env()[t.urtype.name]}
    elif t.node_type == 'Type_Specialized':
        extern_name = t.baseType.path.name

        # TODO is there a more straightforward way to deal with such externs?
        argtyped_externs = ["Digest"]

        if extern_name in argtyped_externs:
            #[ ${append_type_postfix(t.arguments[0].urtype.name)}
        else:
            # types for externs (smems)

            if 'baseType' in t:
                constructor = t.urtype.constructors[0]
                datatype = constructor.type.parameters.parameters[0].urtype
                #= format_type(datatype)
            else:
                int_type = 'int' if t.arguments[0].isSigned else 'uint'
                bitsize = t.arguments[0].size
                size = sizeup(bitsize, use_array)

                #[ ${t.baseType.path.name}_${int_type}${size}_t
    elif t.node_type == 'Type_Bits':
        size = sizeup(t.size, use_array)
        base = f'rte_atomic{size}_t' if is_atomic else f'int{size}_t' if t.isSigned else f'uint{size}_t'
        if use_array:
            #[ $base $addon[(${t.size} + 7) / 8]
        else:
            ptr = '*' if t.size > 32 else ''
            #[ $base$ptr $addon
    elif t.node_type == 'Type_Enum':
        if is_atomic:
            name = 'rte_atomic32_t'
        else:
            name = t.c_name if 'c_name' in t else t.name
            name = append_type_postfix(add_prefix('enum_', name))
        #[ $name $addon
    elif t.node_type == 'Type_Name':
        t2 = t.urtype
        if not resolve_names and 'name' in t2:
            #[ ${t2.name}
        else:
            #= gen_format_type(t2, resolve_names, use_array, addon)
    elif t.node_type == 'Type_Extern':
        is_smem = 'smem_type' in t
        if not is_smem:
            #[ EXTERNTYPE(${t.name})
        elif (reg := t).smem_type in ('register'):
            signed = 'uint' if reg.repr.isSigned else 'int'
            size = align8_16_32(reg.repr.size)
            #[ REGTYPE($signed,$size)
        else:
            #[ SMEMTYPE(${t.smem_type})
    else:
        addError('formatting type', f'Type {t.node_type} for node ({t}) is not supported yet')
        #[ TODO_TYPE_FOR_${t.name} /* placeholder type for ${t.node_type} */

def gen_format_type_mask(t):
    if t.node_type == 'Type_Bits' and not t.isSigned:
        if t.size <= 32:
            var = generate_var_name(f"bitmask_{t.size}b")
            mask = hex((2 ** t.size) - 1)
            masksize = 8 if t.size <= 8 else 16 if t.size <= 16 else 32
            #pre[ uint${masksize}_t $var = $mask;
            #[ $var
        else:
            addError('formatting a type mask', f'Masked type is {t.size} bits, only 32 or less is supported')
    else:
        addError('formatting a type mask', 'Currently only bit<w> is supported!')

def gen_format_declaration(d, varname_override):
    var_name = d.name if varname_override is None else varname_override

    if d.node_type == 'Declaration_Variable':
        if d.urtype.node_type == 'Type_Header':
            # Data for variable width headers is stored in parser_state_t
            pass
        elif d.urtype.node_type == 'Type_Boolean':
            #[ bool ${var_name} = false;
        else:
            t = gen_format_type(d.type, False)
            #[ $t ${var_name};
    elif d.node_type == 'Declaration_Instance':
        t = f'{gen_format_type(d.type, False)}_t'
        #[ extern void ${t}_init(${t}*);
        #[ $t ${var_name};
        #[ ${t}_init(&${var_name});
    elif d.node_type in ('P4Table', 'P4Action'):
        #[ /* nothing */
    else:
        addError('formatting declaration', f'Declaration of type {d.node_type} is not supported yet!')

################################################################################

def int_to_big_endian_byte_array_with_length(value, width, base=10):
    array = []
    while value > 0:
        array.append(int(value % 256))
        value /= 256
    array.reverse()
    array_len = len(array)
    padded_array = [0 for i in range(width-array_len)] + array[array_len-min(array_len, width) : array_len]
    return '{' + ', '.join([with_base(x, base) for x in padded_array]) + '}'


def bit_bounding_unit(t):
    """The bit width of the smallest int that can contain the type,
    or the string "bytebuf" if it is larger than all possible container types."""
    return 'bytebuf' if t.size > 32 else f'"{align8_16_32(t.size)}"'


def member_to_hdr_fld(member_expr):
    hdrname = member_expr.hdr_ref.name
    if 'fld_ref' not in member_expr:
        return f'HDR({hdrname})', None

    fldname = member_expr.fld_ref.name
    return f'HDR({hdrname})', f'FLD({hdrname},{fldname})'


def gen_format_statement_fieldref_wide(dst, src, dst_width, dst_is_vw, dst_bytewidth, dst_name, dst_hdrname, dst_fldname):
    if src.node_type == 'Member':
        src_pointer = generate_var_name('tmp_fldref')
        #[ uint8_t $src_pointer[$dst_bytewidth];

        hdrname, fldname = get_hdrfld_name(src)

        #[ GET_BUF(${src_pointer}, dst_pkt(pd), FLD($hdrname, $fldname));
        if dst_is_vw:
            src_vw_bitwidth = f'pd->headers[HDR({hdrname})].vw_size'
            dst_bytewidth = f'({src_vw_bitwidth}/8)'
    elif src.node_type == 'PathExpression':
        name = src.path.name
        refbase = "local_vars->" if is_control_local_var(name) else 'parameters.'
        src_pointer = f'{refbase}{name}'
    elif src.node_type == 'Constant':
        src_pointer = generate_var_name('tmp_fldref_const')
        #[ uint8_t $src_pointer[$dst_bytewidth] = ${int_to_big_endian_byte_array_with_length(src.value, dst_bytewidth, src.base)};
    elif src.node_type == 'Mux':
        src_pointer = generate_var_name('tmp_fldref_mux')
        #[ uint8_t $src_pointer[$dst_bytewidth] = ((${format_expr(src.e0.left)}) == (${format_expr(src.e0.right)})) ? (${format_expr(src.e1)}) : (${format_expr(src.e2)});
    elif src.node_type in elementwise_binary_ops:
        src_pointer = generate_var_name(f'tmp_{src.node_type}')
        op = elementwise_binary_ops[src.node_type];
        #[ uint8_t $src_pointer[$dst_bytewidth];
        #{ for (int i = 0; i < $dst_bytewidth; ++i) {
        #[     $src_pointer[i] = (${format_expr(src.left)})[i] $op (${format_expr(src.right)})[i];
        #} }
    else:
        src_pointer = 'NOT_SUPPORTED'
        addError('formatting statement', f'Assignment to unsupported source type {src.node_type} in: {format_expr(dst)} = {src}')

    dst_fixed_size = dst.expr.hdr_ref.urtype.size - dst.fld_ref.size

    if dst_is_vw:
        #[ pd->headers[${dst_hdrname}].vw_size = ${src_vw_bitwidth};
        #[ pd->headers[${dst_hdrname}].size = ($dst_fixed_size + pd->headers[${dst_hdrname}].vw_size)/8;

    #[ set_fld_buf(pd, $dst_fldname, $src_pointer);

def is_primitive(typenode):
    """Returns true if the argument node is compiled to a non-reference C type."""
    # TODO determine better if the source is a reference or not
    return typenode.node_type == "Type_Boolean" or (typenode.node_type == 'Type_Bits' and typenode.size <= 32)


def gen_format_statement_fieldref_short(dst, src, dst_width, dst_is_vw, dst_bytewidth, dst_name, dst_hdrname, dst_fldname):
    bitlen = 8 if dst.urtype.size <= 8 else 16 if dst.urtype.size <= 16 else 32
    bytelen = 1 if dst.urtype.size <= 8 else 2 if dst.urtype.size <= 16 else 4
    varname = generate_var_name(f'value{bitlen}b')
    if src.node_type == 'PathExpression':
        indirection = "&" if is_primitive(src.type) else ""
        var_name = src.path.name
        refbase = "local_vars->" if is_control_local_var(src.decl_ref.name) else 'parameters.'

        #[ uint${bitlen}_t ${varname};
        if refbase == "local_vars->":
            #[ memcpy(&$varname, $indirection($refbase${var_name}), $dst_bytewidth);
            if bitlen > 8:
                #[ $varname = t4p4s2net_${bytelen}(${varname});
        else:
            #[ memcpy(&$varname, $indirection($refbase${var_name}), $dst_bytewidth);
    else:
        #[ uint${bitlen}_t $varname = ${format_expr(src)};

    #{ if (likely(is_header_valid(${dst_hdrname}, pd))) {
    #[     set_fld(pd, $dst_fldname, $varname);
    #[ } else {
    #[     debug("   " T4LIT(!!,warning) " Ignoring assignment to field in invalid header: " T4LIT(%s,warning) "." T4LIT(%s,field) "\n", hdr_infos[$dst_hdrname].name, field_names[$dst_fldname]);
    #} }


def gen_format_statement_fieldref(dst, src):
    #TODO: handle preparsed fields, width assignment for vw fields and assignment to buffer instead header fields
    dst_width = dst.type.size
    dst_is_vw = dst.type.node_type == 'Type_Varbits'
    dst_bytewidth = (dst_width+7)//8

    assert(dst_width == src.type.size)
    assert(dst_is_vw == (src.type.node_type == 'Type_Varbits'))

    dst_name = dst.expr.member if dst.expr.node_type == 'Member' else dst.expr.path.name if dst.expr('hdr_ref', lambda h: h.urtype.is_metadata) else dst.expr._hdr_ref._path.name
    dst_hdrname, dst_fldname = member_to_hdr_fld(dst)

    if dst_width <= 32:
        #= gen_format_statement_fieldref_short(dst, src, dst_width, dst_is_vw, dst_bytewidth, dst_name, dst_hdrname, dst_fldname)
    else:
        #= gen_format_statement_fieldref_wide(dst, src, dst_width, dst_is_vw, dst_bytewidth, dst_name, dst_hdrname, dst_fldname)


def is_atomic_block(blckstmt):
    try:
        return any(blckstmt.annotations.annotations.filter('name', 'is_atomic'))
    except:
        return False


def format_source(src):
    nt = src.node_type
    if nt == 'Constant':
        return f'T4LIT({src.value})'
    if nt == 'Member':
        hdrname, fldname = get_hdrfld_name(src)
        return f'T4LIT({hdrname},header) "." T4LIT({fldname},field)'
    if nt == 'MethodCallExpression':
        return f'T4LIT({src.method.expr.path.name}) "." T4LIT({src.method.member}) "()"'
    if nt == 'PathExpression':
        return f'T4LIT({src.path.name},field)'
    if nt == 'Cast':
        if src.expr.node_type in simple_binary_ops:
            return f'T4LIT(simple-binary-op,field)'
        if src.expr.node_type in complex_binary_ops:
            return f'T4LIT(complex-binary-op,field)'
        if src.expr.node_type in elementwise_binary_ops:
            return f'T4LIT(elementwise-binary-op,field)'
        return format_source(src.expr)
    if nt == 'Mux':
        return f'"("{format_source(src.e0)} " ? " {format_source(src.e1)} " : " {format_source(src.e2)} ")"'
    if nt in simple_binary_ops:
        op = simple_binary_ops[nt]
        return f'{format_source(src.left)} "{op}" {format_source(src.right)}'
    if nt in complex_binary_ops:
        op = complex_binary_ops[nt]
        return f'{format_source(src.left)} "{op}" {format_source(src.right)}'

    addWarning('formatting source', f'Unexpected {nt} source found')
    return 'unknown'


def gen_do_assignment(dst, src, ctl=None):
    if dst.type.node_type == 'Type_Header':
        src_hdrname = src.path.name if src.node_type == 'PathExpression' else src.member
        dst_hdrname = dst.path.name if dst.node_type == 'PathExpression' else dst.member

        #[ do_assignment(HDR(${dst_hdrname}), HDR(${src_hdrname}), SHORT_STDPARAMS_IN);
    elif dst.type.node_type == 'Type_Bits':
        # TODO refine the condition to find out whether to use an assignment or memcpy
        requires_memcpy = src.urtype.size > 32 or 'decl_ref' in dst
        # is_assignable = src.type.size in [8, 32]
        is_assignable = src.urtype.size <= 32

        if src.type.node_type == 'Type_Bits' and not requires_memcpy:
            if is_assignable:
                if dst("expr.hdr_ref.urtype.is_metadata") or dst("hdr_ref.urtype.is_metadata"):
                    fldname = dst.member if dst("expr.hdr_ref.urtype.is_metadata") else dst.field_name
                    #[ set_fld(pd, FLD(all_metadatas,$fldname), ${format_expr(src, expand_parameters=True)});
                elif dst.node_type == 'Slice':
                    #[ // TODO assignment to slice
                    addWarning('Compiling assignment', 'Assignment to slice is currently not supported')
                else:
                    if dst.node_type == 'Member':
                        if dst.urtype('is_metadata', False):
                            # Note: the metadata header and field name is joined by underscores, separating them as best as possible
                            hdrname = "all_metadatas"
                            fldname = nameparts[-1]
                        else:
                            hdrname, fldname = get_hdrfld_name(dst)

                        if dst.urtype.size <= 32:
                            # (${format_type(dst.type)})(${format_expr(src, expand_parameters=True)})
                            is_local = 'path' in src and 'name' in src.path and is_control_local_var(src.path.name)
                            srcbuf = casting(dst.type, is_local, format_expr(src, expand_parameters=True))
                            #[ MODIFY(dst_pkt(pd), FLD($hdrname,$fldname), src_32($srcbuf), ENDIAN_KEEP);
                        else:
                            #TODO
                            pass

                        #[ debug("       : " T4LIT($hdrname,header) "." T4LIT($fldname,field) " = " T4LIT(%d,bytes) " (" T4LIT(%${(src.type.size+7)//8}x,bytes) ")\n", ${format_expr(dst)}, ${format_expr(dst)});
                    else:
                        # TODO the printout should contain the local variable's name, not its C representation
                        locvar = format_expr(dst)
                        #[ debug("       : " T4LIT($locvar,field) " = " T4LIT(%d,bytes) " (" T4LIT(%${(src.type.size+7)//8}x,bytes) ")\n", ${format_expr(dst)});

                        if dst.urtype.size <= 32:
                            #[ ${format_expr(dst)} = (${format_type(dst.type)})(${format_expr(src, expand_parameters=True)}));
                        else:
                            #TODO
                            pass
            else:
                if 'decl_ref' in dst:
                    addError("Assigning variable", f"Variable {dst.path.name} is larger than 32 bits, assignment currently unsupported")
                    #[ // TODO something like: ${format_expr(dst)} = ${format_expr(src)};
                else:
                    hdr = dst.expr.hdr_ref
                    fldname = dst.member

                    if dst.type.size <= 32:
                        #[ ${format_expr(dst)} = ${format_expr(src)};
                    else:
                        tmpvar = generate_var_name('assignment')
                        #[ ${format_type(dst.type)} $tmpvar = (${format_type(dst.type)})(${format_expr(src, expand_parameters=True, needs_variable=True)});
                        #[ MODIFY(dst_pkt(pd), FLD(${hdr.name},${fldname}), src_buf(&$tmpvar, sizeof(${format_type(dst.type)})), ENDIAN_NET);
                        #[ dbg_bytes(get_handle_fld(pd, FLD(${hdr.name},${fldname}), "debug").pointer, sizeof(${format_type(dst.type)}),
                        #[           "        : "T4LIT(${hdr.name},header)"."T4LIT(${fldname},field)"/"T4LIT(%zuB)" = ",
                        #[           sizeof(${format_type(dst.type)}));
        else:
            srcexpr = format_expr(src, expand_parameters=True, needs_variable=True)
            with SugarStyle('no_comment_inline'):
                # TODO replace using C code as debug output with better formatting
                dsttxt = gen_format_expr(dst).strip()
                srctxt = format_source(src)

            size = (dst.type.size+7)//8
            pad_size = 4 if size == 3 else size
            net2t4p4s = '' if size > 4 else f'net2t4p4s_{pad_size}'

            tmpvar = generate_var_name('assignment')

            if dst.node_type == 'Member':
                hdrname = dst.expr.hdr_ref.name
                fldname = dst.member
                #[ ${format_type(dst.type)} $tmpvar = $net2t4p4s((${format_type(dst.type)})(${format_expr(src, expand_parameters=True, needs_variable=True)}));
                #[ dbg_bytes(&($srcexpr), $size, "    : Set " T4LIT($hdrname,header) "." T4LIT($fldname,field) "/" T4LIT(${size}B) " = " $srctxt " = ");
                #[ MODIFY(dst_pkt(pd), FLD($hdrname,$fldname), src_buf(&$tmpvar, $size), ENDIAN_NET);
            else:
                nt = src.node_type
                is_op_node = nt in simple_binary_ops or nt in complex_binary_ops or nt in elementwise_binary_ops
                is_local = nt == 'PathExpression'
                is_nonref_node = is_op_node or nt in ('Constant', 'MethodCallExpression', 'Cast') or is_local
                is_ref_node = is_op_node or nt in ('Constant', 'Member', 'MethodCallExpression', 'Cast') or is_local

                needs_dereferencing = not is_nonref_node and size <= 4
                deref = "*" if needs_dereferencing else ""
                #[ ${format_type(dst.type)} $tmpvar = $deref(${format_type(dst.type)}$deref)((${format_expr(src, expand_parameters=True, needs_variable=True)})); // dbg ${is_op_node} ${is_local} ${is_nonref_node} ${nt}->${dst.node_type}

                needs_referencing = is_ref_node and size <= 4
                ref = f'&' if needs_referencing else f''
                #[ memcpy(&(${format_expr(dst)}), $ref$tmpvar /* dbg ${is_op_node} ${is_local} ${is_nonref_node} ${nt}->${dst.node_type} */, $size);

                dstname = dst.path.name
                short_name = ctl.locals.get(dstname, 'Declaration_Variable').short_name
                #[ dbg_bytes(&(${format_expr(dst)}), $size, "    : Set " T4LIT(${short_name},field) "/" T4LIT(${size}B) " = " $srctxt " = ");
    elif dst.node_type == 'Member':
        tmpvar = generate_var_name('assign_member')
        hdrname = dst.expr.hdr_ref.name
        fldname = dst.member

        #pre[ ${format_type(dst.type)} $tmpvar = ${format_expr(src, expand_parameters=True)};
        #[ MODIFY(dst_pkt(pd), FLD($hdrname,$fldname), src_buf(&$tmpvar, sizeof(${format_type(dst.type)})), ENDIAN_CONVERT_AS_NEEDED);
    else:
        #[ ${format_expr(dst)} = ${format_expr(src, expand_parameters=True)};


def is_extract(m):
    return m.member == 'extract' and len(m.type.parameters.parameters) == 1 and m.expr.type.name == 'packet_in'

def gen_format_statement(stmt, ctl=None):
    if stmt.node_type == 'AssignmentStatement':
        dst = stmt.left
        src = stmt.right
        if 'fld_ref' in dst:
            #= gen_format_statement_fieldref(dst, src)
        else:
            #= gen_do_assignment(dst, src, ctl)
    elif stmt.node_type == 'BlockStatement':
        is_atomic = is_atomic_block(stmt)
        if is_atomic:
            #[ LOCK(&${stmt.enclosing_control.type.name}_lock)
        for c in stmt.components:
            #= gen_format_statement(c, ctl=ctl)
        if is_atomic:
            #[ UNLOCK(&${stmt.enclosing_control.type.name}_lock)
    elif stmt.node_type == 'IfStatement':
        cond = format_expr(stmt.condition)

        # note: the condition may create/use some constants
        pre, post = compiler_common.statement_buffer_value()

        if cond.strip() == '':
            cond = "true"

        #[ ${pre}
        #{ if ($cond) {
        #=     format_statement(stmt.ifTrue, ctl=ctl)
        if 'ifFalse' in stmt:
            #[ } else {
            #=     format_statement(stmt.ifFalse, ctl=ctl)
        #} }
        #[ ${post}
    elif stmt.node_type == 'MethodCallStatement':
        mcall = stmt.methodCall
        m = mcall.method

        if 'member' in m and is_extract(m):
            ee = stmt.methodCall.arguments[0].expression
            if (is_stack := (ee.expr.urtype.node_type == 'Type_Stack')):
                # TODO fix in hlir_attrs: ee.hdr_ref and ee.expr.hdr_ref should not be None even for a stack
                name = ee.expr.member
                hdrcode = f'stk_current(STK({name}), pd)'
                max_stkhdr_count = f'{ee.expr.urtype.size.value}'
            else:
                name = ee.hdr_ref.name
                hdrcode = f'HDR({name})'
                max_stkhdr_count = f'-1 /* ignored */'

            vw = generate_var_name('vw')
            result_len = generate_var_name(f'hdrlen{f"_stk" if is_stack else ""}_{name}')

            #pre[ int parser_extract_$name(int, STDPARAMS);
            #pre[ int $vw = hdr_infos[$hdrcode].var_width_field;
            #[ int ${result_len} = parser_extract_$name($vw == NO_VW_FIELD_PRESENT ? 0 : fld_infos[$vw].byte_width * 8, STDPARAMS_IN);
            #{ if (unlikely(${result_len} < 0)) {
            #[     gen_parse_drop_msg(${result_len}, "${name}", ${max_stkhdr_count});
            #[     drop_packet(STDPARAMS_IN);
            #[     return false;
            #} }

            # TODO activate or remove
            # { if (unlikely(${result_len} < 0)) {
            # [     drop_packet(STDPARAMS_IN);
            # [     return;
            # } }
        elif m.node_type == 'Method' and m.name == 'digest':
            #[ ${gen_format_methodcall_digest(m, mcall)};
        elif 'member' in m and not is_extract(m):
            #[ ${gen_fmt_methodcall(mcall, m)};
        elif 'expr' in m and m.expr.urtype.node_type == 'Type_Stack':
            #[ /* TODO stack */
        else:
            #[ ${gen_short_extern_call(mcall)};
    elif stmt.node_type == 'SwitchStatement':
        #[ switch (${format_expr(stmt.expression)}) {
        for case in stmt.cases:
            if case.label.node_type == "DefaultExpression":
                #[ default:
            else:
                #[ case ${format_expr(case.label)}:
            #[   ${format_statement(case.statement)}
            #[   break;
        if [case for case in stmt.cases if case.label.node_type == "DefaultExpression"] == []:
            #[   default: {}
        #[ }

def gen_format_methodcall_digest(m, mcall):
    digest_name = mcall.typeArguments[0].name
    port, fields = mcall.arguments

    #[ struct type_field_list fields;
    #[ fields.fields_quantity = ${len(fields)};
    #[ fields.field_ptrs   = malloc(sizeof(uint8_t*)*fields.fields_quantity);
    #[ fields.field_widths = malloc(sizeof(uint8_t*)*fields.fields_quantity);

    for idx, fld in enumerate(fields.components):
        if fld.expr.type.is_metadata:
            hdrname, fldname = fld.expr.name,   fld.member
        else:
            hdrname, fldname = fld.expr.member, fld.expression.fld_ref.name

        #[ fields.field_ptrs[$idx]   = (uint8_t*) get_fld_pointer(pd, FLD($hdrname,$fldname));
        #[ fields.field_widths[$idx] =            fld_infos[FLD($hdrname,$fldname)].size;
    #[ generate_digest(bg,"${digest_name}",0,&fields);
    #[ sleep_millis(DIGEST_SLEEP_MILLIS);

def is_emit(m):
    return m.expr._ref.urtype('name', lambda n: n == 'packet_out')

def gen_isValid(hdrname):
    #[ controlLocal_tmp_0 = is_header_valid(HDR($hdrname), pd);

def gen_setValid(hdrname):
    #[ set_hdr_valid(HDR($hdrname), SHORT_STDPARAMS_IN);

def gen_setInvalid(hdrname):
    #[ set_hdr_invalid(HDR($hdrname), SHORT_STDPARAMS_IN);


def gen_emit(mcall):
    arg = mcall.arguments[0]
    ae = arg.expression

    hdrname = get_hdr_name(ae)
    hdr_type = ae.type

    # TODO remove?
    # hdrname = ae.hdr_ref.name if 'hdr_ref' in ae else ae.member

    #[ pd->header_reorder[pd->deparse_hdrinst_count] = HDR($hdrname);
    #[ ++pd->deparse_hdrinst_count;

def gen_stk_push_front(stk, mcall):
    count = mcall.arguments[0].expression.value
    #[ /* TODO finish stack pop_front */
    for invalidated_idx in range(count):
        #[ set_hdr_invalid(stk_at_idx(STK($stk), ${invalidated_idx}, pd), SHORT_STDPARAMS_IN);

def gen_stk_pop_front(hdrname, mcall):
    #[ /* TODO stack pop_front */

def gen_fmt_methodcall(mcall, m):
    specials = ('isValid', 'setValid', 'setInvalid')
    if is_emit(m):
        #= gen_emit(mcall)
        #[ /* done calling gen_emit */
    elif 'table_ref' in m.expr and m.member == 'apply':
        #[ ${gen_method_apply(mcall)};
    elif m.member in specials:
        hdrname, op = get_hdrfld_name(m)
        #= gen_$op(hdrname)
        #[ /* done call for handling dedicated call: $op */
    elif 'member' not in m.expr:
        #= gen_fmt_methodcall_extern(m, mcall)
        #[ /* done calling extern */
    elif 'expr' in m and m.expr.urtype.node_type == 'Type_Stack':
        stk, op = get_hdrfld_name(m)
        #= gen_stk_$op(stk, mcall)
        #[ /* done calling stack operation $op */
    else:
        #[ ${gen_short_extern_call(mcall)}

def gen_fmt_methodcall_extern(m, mcall):
    # TODO treat smems and digests separately
    # currently, for legacy reasons, smem_type can take the value 'Digest'
    dref = m.expr.decl_ref
    smem_type = dref.smem_type if 'decl_ref' in m.expr and 'smem_type' in dref else m.expr.urtype.name

    # if the extern is about both packets and bytes, it takes two separate calls
    is_possibly_multiple = smem_type in ("counter", "meter", "direct_counter", "direct_meter")
    if is_possibly_multiple:
        pobs, reverse_pobs = packets_by_model(compiler_common.current_compilation['hlir'])
        if dref.packets_or_bytes == 'packets_and_bytes':
            #= gen_format_extern_single(m, mcall, smem_type, is_possibly_multiple, pobs["packets"])
            #= gen_format_extern_single(m, mcall, smem_type, is_possibly_multiple, pobs["bytes"])
        else:
            #= gen_format_extern_single(m, mcall, smem_type, is_possibly_multiple, pobs[dref.packets_or_bytes])
    else:
        #= gen_format_extern_single(m, mcall, smem_type, is_possibly_multiple)


def gen_smem_expr(smem, packets_or_bytes):
    if smem.type.node_type == 'Type_Specialized':
        if 'packets_or_bytes' in smem and smem.packets_or_bytes == 'packets_and_bytes':
            pobs, reverse_pobs = packets_by_model(compiler_common.current_compilation['hlir'])
            revpb = reverse_pobs[packets_or_bytes]
            smem_inst = smem.get_attr(f'smem_{revpb}_inst')
            #= format_expr(smem_inst)
        else:
            #= format_expr(smem)
    else:
        smem_name = smem.urtype.name
        prefix = '' if smem_name == 'Digest' else f'{smem_name}'
        pob = packets_or_bytes or ('packets_or_bytes' in smem and smem.packets_or_bytes)
        # postfix_pob = f'{pob}' if smem_name in ('counter', 'meter', 'direct_counter') else ''
        postfix_table = f'{smem.table.name}' if 'table' in smem else ''
        is_direct = 'is_direct' in smem and smem.is_direct
        postfix_idx = '[0]'

        name_parts = [part for part in (prefix, smem.name, postfix_table) if part != '']
        #[ &(global_smem.SMEM${len(name_parts)}(${','.join(name_parts)})${postfix_idx})

def get_pob_arg_idx(args, m, packets_or_bytes):
    pobs = [idx for idx, ae in enumerate(args.map('expression')) if ae.node_type not in ('Constant') if ae.member in ('packets', 'bytes', 'packets_or_bytes', 'packets_and_bytes', packets_or_bytes)]
    return pobs[0] if pobs else 1

def replace_pob(m, funargs_pre, packets_or_bytes):
    args = m.expr.decl_ref.arguments
    pob_arg_idx = get_pob_arg_idx(args, m, packets_or_bytes)
    smem_type = args[pob_arg_idx].expression.type
    prefix = f'enum_{smem_type.name}'
    return [f'{prefix}_{packets_or_bytes}' if idx == pob_arg_idx else arg for idx, arg in enumerate(funargs_pre)]

def format_type_ref(arg, is_ref):
    ref = '*' if is_ref else ''
    return f'{format_type(arg.expression.type, addon = ref)}'

def format_expr_ref(arg, var, is_ref):
    if is_ref:
        if arg.expression.node_type in ('PathExpression'):
            return f'&({format_expr(arg.expression)})'
        return f'&{var}'
    return format_expr(arg.expression)

ExternCallProblemType = Enum('ExternCallProblemType', 'no_msg_all_ok msg_idx_too_high', start=0)

# TODO move this to one of the headers, then use it in gen_format_extern_single
#{ enum {
for ecpt in ExternCallProblemType:
    #[     $ecpt,
#} } ExternCallProblemType_e;
#[

def gen_extern_call_problem_type(m, mcall, smem_type, is_possibly_multiple, dref, args, packets_or_bytes = None):
    amount_idxs = {
        ('register', 'read'): 2,
        ('register', 'write'): 1,
    }

    smem_info = (smem_type, m.member)
    if smem_info in amount_idxs:
        amount_idx = amount_idxs[smem_info]
        arg, var, is_ref = args[amount_idx]
        amount_type = format_type(arg.expression.type)
        #[ ((${amount_type})(${dref.amount}) /* max. amount */ < ${var} /* actual amount */ ? ${ExternCallProblemType.msg_idx_too_high.value} : ${ExternCallProblemType.no_msg_all_ok.value})
    else:
        #[ ${ExternCallProblemType.no_msg_all_ok.value}

def get_short_type(n):
    if n.node_type == 'Type_InfInt':
        return f'{inf_int_short_c_type()}'
    if n.node_type == 'Type_List':
        return f'{get_short_type(n.components[0])}s'
    if n.node_type == 'Type_Bits':
        if n.size > 32:
            return 'buf'
        sign = 'i' if n.isSigned else 'u'
        size = '8' if n.size <= 8 else '16' if n.size <= 16 else '32'
        return f'{sign}{size}'
    return n.name

def get_mcall_type_args(mcall):
    if 'type_parameters' in (e := mcall.method.expr):
        return P4Node(e.type_parameters)
    extern_type = [mcall.method.expr.urtype] if 'expr' in mcall.method else []
    method_type = [mcall.method.urtype]
    return P4Node(extern_type + method_type).flatmap('typeParameters.parameters').filter('node_type', ('Type_Var'))

def gen_format_extern_single(m, mcall, smem_type, is_possibly_multiple, packets_or_bytes = None):
    extern_name = m.expr.urtype.name
    if (m.expr.decl_ref.urtype.name, mcall.method.member) == ('packet_in', 'advance'):
        size = mcall.arguments[0].expression.value
        if size % 8 != 0:
            size2 = ( (size+7)//8 ) * 8
            addWarning('Advancing', f'Asked to advance {size} bits which is not byte aligned, advancing {size2 // 8} bytes instead')
            size = size2

        #[ pd->extract_ptr += ($size+7) / 8;
        #[ pd->is_deparse_reordering = true;
        #[ debug("   :: " T4LIT(Advancing packet,status) " by " T4LIT(${(size+7)//8}B) "\n");
    else:
        dref = mcall.method.expr.decl_ref

        args = [(arg, generate_var_name('declarg'), False) for arg in dref.arguments] + [(arg, generate_var_name(f'arg_{par.name}'), par.direction != 'in') for arg, par in zip(mcall.arguments, mcall.method.type.parameters.parameters)]

        funargs = [var for arg, var, is_ref in args]
        argtypes = [format_type_ref(arg, is_ref) for arg, var, is_ref in args]

        if packets_or_bytes is not None:
            funargs = replace_pob(m, funargs, packets_or_bytes)

        if (extern := dref.urtype).node_type == 'Type_Extern' and 'smem_type' not in extern:
            smemref = []
            externtype, externref = [f'{extern_name}*'], [f'&global_smem.EXTERNNAME({dref.name})']
        else:
            smemref = [gen_smem_expr(dref, packets_or_bytes)]
            externtype, externref = [], []

        funargs = ", ".join(externref + list(funargs) + smemref + ['SHORT_STDPARAMS_IN'])
        targs = dref.type('arguments', [])
        argtypes = ", ".join(externtype + list(argtypes) + [f'{format_type(dref.urtype, type_args = targs)}*', 'SHORT_STDPARAMS'])

        def find_param_type(mcall_type_arg):
            if 'type_ref' in mcall_type_arg and (tref := mcall_type_arg.type_ref).node_type == 'Parameter':
                margs = mcall.method.urtype.parameters.parameters
                marg = margs.get(tref.name, 'Parameter')
                argidx = margs.vec.index(marg)

                return mcall.arguments[argidx].expression.urtype

            return mcall_type_arg.urtype

        type_args_postfix_parts = get_mcall_type_args(mcall).map(find_param_type).map(get_short_type)

        funname = f'EXTERNCALL{len(type_args_postfix_parts)}({extern_name},{",".join([m.member] + list(type_args_postfix_parts))})'

        for arg, var, is_ref in args:
            argexpr = arg.expression
            if is_ref and argexpr.node_type == 'Member':
                content = generate_var_name('content')
                #[ ${format_type(argexpr.type)} $content = ${format_expr(arg)};
                #[ ${format_type(argexpr.type)}* $var = &$content;
            else:
                ref, ptr = ('&', '*') if is_ref else ('', '')
                #[ ${format_type(argexpr.type)}$ptr $var = $ref${format_expr(arg)};

        problem = generate_var_name('extern_call_problem')
        with SugarStyle("inline_comment"):
            problem_code = gen_extern_call_problem_type(m, mcall, smem_type, is_possibly_multiple, dref, args, packets_or_bytes = None)

        formatted_args = args[2:]
        vars_txt = ''.join(f', {var}' for arg, var, is_ref in formatted_args)

        dollar = '$'
        #[ int $problem = ${problem_code};

        #{ if (likely($problem == 0)) {
        #[     $funname($funargs);
        #[ } else {
        #{     #ifdef T4P4S_DEBUG
        #[         char cause_txt[256];

        if smem_type == 'register':
            #{         if ($problem == ${ExternCallProblemType.msg_idx_too_high.value}) {
            #[             sprintf(cause_txt, "register index (" T4LIT(%1${dollar}d = 0x%1${dollar}x) ") is too high" ${vars_txt});
            #}         }

        #[         debug("   " T4LIT(!!,warning) " Call to extern " T4LIT($extern_name) " was " T4LIT(denied,warning) ", cause: %s\n", cause_txt);
        #}     #endif
        #} }
        for arg, var, is_ref in args:
            if not is_ref:
                continue
            if arg.expression.node_type in ('PathExpression'):
                continue
            ae = arg.expression
            hdrname = ae.expr.member if 'expr' in ae and 'member' in ae.expr else ae.expr.hdr_ref.name
            fldname = ae.member
            #[ MODIFY(dst_pkt(pd), FLD($hdrname,$fldname), src_32(*$var), ENDIAN_KEEP);


def is_ref(node):
    not_of_type   = node.node_type not in ('Constant', 'BoolLiteral', 'MethodCallExpression', 'StructExpression', 'StringLiteral')
    not_of_urtype = node.urtype.node_type not in ('Type_Error', 'Type_Enum', 'Type_List')
    return not_of_type and not_of_urtype


def gen_short_extern_call(mcall):
    m = mcall.method

    def format_with_ref(e):
        ut = e.urtype
        if ut.node_type in ('Type_List'):
            return 'uint8_buffer_t'
        if ut.node_type in ('Type_Header'):
            return 'bitfield_handle_t' if not ut.is_metadata else None
        ref = "*" if is_ref(e) else ""
        return f'{format_type(e.type)}{ref}'

    mname_parts, _parinfos, _ret, partypeinfos = get_mcall_infos(mcall)
    partype_suffix = ''.join(f',{partypename}' for partype, partypename in partypeinfos)
    funname = f'SHORT_EXTERNCALL{len(partypeinfos) + len(mname_parts)-1}({",".join(mname_parts)}{partype_suffix})'

    # # TODO clone operations are not supported currently,
    # #      but the call should be generated to an implementation with an empty body
    # if mname.startswith('clone'):
    #     return None

    #[ ${format_expr(mcall, funname_override=funname)};


################################################################################

# A set of expression IDs that have already been generated.
generated_exprs = set()

def convert_component(component):
    if component.node_type == 'Member':
        hdr      = component.expr
        fld_name = component.member
        fld      = hdr.type.fields.get(fld_name)
        return (component.node_type, hdr, fld)

    if component.node_type == 'Constant':
        return (component.node_type, component.value, "")

    addWarning('generating list expression buffer', f'Skipping not supported list element {component}')
    return None

################################################################################

def gen_method_hdr_ref(e):
    ee = e.method.expr
    if 'hdr_ref' in ee:
        #[ HDR(${ee.member})
    elif ee.node_type == 'ArrayIndex':
        #[ HDR(${get_hdr_name(ee)})
    else:
        #= format_expr(ee)


def gen_method_isValid(e):
    #[ is_header_valid(${gen_method_hdr_ref(e)}, pd)

def gen_method_setInvalid(e):
    #[ !is_header_valid(${gen_method_hdr_ref(e)}, pd)

def gen_method_apply(e):
    action = e.method.expr.path.name
    #[ ${action}_apply(STDPARAMS_IN)

def gen_method_lookahead(e):
    arg0 = e.typeArguments[0]
    size = arg0.size

    if size <= 32:
        size8 = align8_16_32(size) // 8

        type = gen_format_type(arg0)

        var = generate_var_name('lookahead')
        var2 = generate_var_name('lookahead_masked')
        #pre[ $type $var = *($type*)(pd->extract_ptr);
        #pre[ $type $var2 = net2t4p4s_$size8(topbits_$size8(t4p4s2net_$size8($var), $size));
        #[ $var2
    else:
        addWarning('doing lookahead', f'Lookahead was called on a type that is {size} bits long; maximum supported length is 32')
        #[ 0xdeadc0de /* temporary ${size}b lookahead placeholder */

def gen_method_setValid(e):
    hdr = e.method.expr.hdr_ref

    init_vw_size = 0
    size = '+'.join(fld.size if not fld.is_vw else init_vw_size for fld in h.urtype.fields)

    #[ pd->headers[${hdr.name}] = (header_descriptor_t) {
    #[     .type = ${hdr.name},
    #[     .size = (($size) + 7) / 8,
    #[     .pointer = calloc(${hdr.urtype.byte_width}, sizeof(uint8_t)),
    #[     .vw_size = ${init_vw_size},
    #[ };


def gen_casting(dst_type, is_local, expr_str):
    dt   = format_type(dst_type)
    varname = generate_var_name('casting')
    # TODO is dereferencing needed here at all?
    needs_dereferencing = False
    if needs_dereferencing:
        #pre[ $dt $varname = *($dt*)($expr_str);
    else:
        #pre[ $dt $varname = ($dt)($expr_str);
    #[ $varname

def gen_masking(dst_type, expr_str):
    dt   = format_type(dst_type)
    mask = format_type_mask(dst_type)
    varname = generate_var_name('masking')
    #pre[ $dt $varname = ($dt)(${casting(dst_type, False, expr_str)});
    #[ ($mask & $varname)

def gen_fmt_Cast(e, format_as_value=True, expand_parameters=False, needs_variable=False, funname_override=None):
    et = e.expr.type
    edt = e.destType
    fe = format_expr(e.expr)
    ft = format_type(edt)
    if (et.node_type, et.size, edt.node_type) == ('Type_Bits', 1, 'Type_Boolean') and not et.isSigned:
        #Cast from bit<1> to bool
        return f"({fe})"
    elif (et.node_type, edt.node_type, edt.size) == ('Type_Boolean', 'Type_Bits', 1) and not edt.isSigned:
        #Cast from bool to bit<1>
        return f'({fe} ? 1 : 0)'
    elif et.node_type == 'Type_Bits' and edt.node_type == 'Type_Bits':
        if et.isSigned == edt.isSigned:
            if not et.isSigned:                       #Cast from bit<w> to bit<v>
                if et.size > edt.size:
                    #[ ${masking(edt, fe)}
                else:
                    #= fe
            else:                                              #Cast from int<w> to int<v>
                #[ (($ft)$fe)
        elif et.isSigned and not edt.isSigned: #Cast from int<w> to bit<w>
            #[ ${masking(edt, fe)}
        elif not et.isSigned and edt.isSigned: #Cast from bit<w> to int<w>
            if edt.size in {8,16,32}:
                #[ (($ft)$fe)
            else:
                addError('formatting an expression', f'Cast from bit<{et.size}> to int<{edt.size}> is not supported! (Only int<8>, int<16> and int<32> are supported.)')
                #[ ERROR_invalid_cast_from_bit${et.size}_to_int${edt.size}
    else:
        #Cast from int to bit<w> and int<w> are performed by P4C
        addError('formatting an expression', f'Cast from {pp_type_16(et)} to {pp_type_16(edt)} is not supported!')
        #[ ERROR_invalid_cast


def gen_fmt_ComplexOp_expr(e, op):
    et = e.type
    with SugarStyle('inline_comment'):
        ltype = format_type(e.left.type)
        lhs = format_expr(e.left)
        rhs = format_expr(e.right)
        #[ ((($ltype)$lhs) $op $rhs)


def gen_fmt_ComplexOp(e, op, format_as_value=True, expand_parameters=False):
    et = e.type
    with SugarStyle('inline_comment'):
        opexpr = gen_fmt_ComplexOp_expr(e, op)
        if e.type.node_type == 'Type_InfInt':
            #[ $opexpr
        elif e.type.node_type == 'Type_Bits':
            if not e.type.isSigned:
                #[ ${masking(e.type, opexpr)}
            elif e.type.size in {8,16,32}:
                #[ ((${format_type(e.type)})$opexpr)
            else:
                addError('formatting an expression', f'Expression of type {e.node_type} is not supported on int<{e.type.size}>. (Only int<8>, int<16> and int<32> are supported.)')
                #[ ERROR


def get_transition_constant_lhs(k):
    if k.node_type == 'PathExpression':
        return f'T4LIT({k.decl_ref.short_name},field)'
    elif k.node_type == 'Member':
        if 'stk_name' in k:
            if k.expr.member == 'last':
                hdrname, fldname, joiner = k.stk_name, k.member, '.last().'
            else:
                addWarning('evaluating stack based condition', 'unexpected condition')
        else:
            hdrname, fldname = get_hdrfld_name(k)
            joiner = '.'

        return f'T4LIT({hdrname},header) "{joiner}" T4LIT({fldname},field)'
    else:
        short_name = k.e0.decl_ref.short_name
        return f'T4LIT({short_name},field) "[" T4LIT({k.e1.value},field) ":" T4LIT({k.e2.value},field) "]"'


def get_transition_constant(value, size, k):
    lhs = get_transition_constant_lhs(k)
    hex_txt = f'"=" T4LIT(0x{value:0{size//4}x},bytes)' if value > 9 else ""
    return f'{lhs} "/" T4LIT({k.urtype.size}) "b=" T4LIT({value}) {hex_txt} " "'


def get_select_conds(select_expr, case):
    cases_tmp = case.keyset.components if case.keyset.node_type == 'ListExpression' else [case.keyset]

    transition_cond_txt = []
    conds = []
    pres = []
    for k, c in zip(select_expr.select.components, cases_tmp):
        select_type = k.type.node_type
        size = k.type.size #if k.type.node_type == 'Type_Bits' else 0
        case_type = c.node_type

        if case_type == 'DefaultExpression':
            conds.append('true /* default */')
        elif case_type == 'Constant' and select_type == 'Type_Bits' and 32 < size and size % 8 == 0:
            byte_array = int_to_big_endian_byte_array_with_length(c.value, size/8)
            pres.append(f'uint8_t {gen_var_name(c)}[{size/8}] = {byte_array};')
            conds.append(f'memcmp({gen_var_name(k)}, {gen_var_name(c)}, {size / 8}) == 0')
        elif size <= 32:
            with SugarStyle("inline_comment"):
                varname = gen_var_name(k)
                if case_type == 'Range':
                    conds.append(f'{format_expr(c.left)} <= {varname} && {varname} <= {format_expr(c.right)}')
                elif case_type == 'Mask':
                    conds.append(f'({format_expr(c.left)} & {varname}) == ({format_expr(c.right)} & {varname})')
                else:
                    if case_type not in {'Constant'}: #Trusted expressions
                        addWarning('formatting a select case', f'Select statement cases of type {case_type} on {pp_type_16(k.type)} might not work properly.')
                    conds.append(f'{varname} == {format_expr(c)}')
                    if case_type == 'Constant':
                        transition_cond_txt.append(get_transition_constant(c.value, c.urtype.size, k))
        else:
            addError('formatting a select case', f'Select statement cases of type {case_type} on {pp_type_16(k.type)} is not supported!')
    return ' && '.join(conds), pres, transition_cond_txt

def gen_fmt_SelectExpression(e, format_as_value=True, expand_parameters=False, needs_variable=False, funname_override=None):
    with SugarStyle("inline_comment"):
        #Generate local variables for select values
        for k in e.select.components:
            varname = gen_var_name(k)
            if k.type.node_type == 'Type_Bits' and k.type.size <= 32:
                deref = '*' if k('needs_dereferencing', True) else ''
                #pre[ ${format_type(k.type)} $varname = $deref(${format_expr(k)});
            elif k.type.node_type == 'Type_Bits' and k.type.size % 8 == 0:
                #pre[ uint8_t $varname[${k.type.size/8}];
                #pre[ GET_BUF($varname, dst_pkt(pd), ${format_expr(k, False)});
            else:
                addError('formatting select expression', f'Select on type {pp_type_16(k.type)} is not supported!')

    if len(e.selectCases) == 0:
        #[ // found a select expression with no cases, no code to generate
        return

    conds_pres = [get_select_conds(e, case) for idx, case in enumerate(e.selectCases)]

    for _, pres, _ in conds_pres:
        for pre in pres:
            #pre[ ${pre}

    for idx, ((cond, _pre, transition_cond_txt), case) in enumerate(zip(conds_pres, e.selectCases)):
        if idx == 0:
            #{ if (${cond}) { // select case #${idx+1}
        else:
            #[ } else if (${cond}) { // select case #${idx+1}

        if transition_cond_txt:
            #[     set_transition_txt(" <== " ${''.join(transition_cond_txt)});

        enclosing_parser = case.parents.filter('node_type', 'P4Parser')[-1]
        #[     parser_state_${enclosing_parser.name}_${case.state.path.name}(STDPARAMS_IN);
    #} }

def gen_fmt_Member(e, format_as_value=True, expand_parameters=False, needs_variable=False, funname_override=None):
    if 'hdr_ref' in e:
        if e.hdr_ref.urtype.is_metadata:
            #[ pd->fields.FLD(${e.expr.member},${e.member})
        else:
            hdrname = e.member if 'member' not in e.expr else e.expr.member
            #[ HDR($hdrname)
    elif e.expr.node_type == 'PathExpression' and e.expr.type.node_type == 'Type_SpecializedCanonical':
        #[ TODO_Type_SpecializedCanonical
    elif e.expr.node_type == 'PathExpression':
        hdr = e.expr.hdr_ref
        if e.expr.urtype.node_type == 'Type_Header':
            fldname = e.member
            size = hdr.urtype.fields.get(e.member).size
            unspec = unspecified_value(size)
            #pre[ check_hdr_valid_${hdr.name}_${fldname}(pd, FLD(${hdr.name},$fldname), "$unspec");
            #[ GET32_def(src_pkt(pd), FLD(${hdr.name},${e.member}), $unspec)
        else:
            is_meta = hdr.urtype('is_metadata', False)
            hdrname = "all_metadatas" if is_meta else hdr.name
            fld = hdr.urtype.fields.get(e.member)
            fldname = e.member
            fldtype = fld.orig_fld.type
            size = fld.size
            unspec = 0 if is_meta else unspecified_value(size)
            var = generate_var_name('member')

            #pre[ check_hdr_valid_${hdrname}_${fldname}(pd, FLD($hdrname,$fldname), "$unspec");
            if size <= 32:
                #pre[ ${format_type(fldtype)} $var = GET32_def(src_pkt(pd), FLD($hdrname,$fldname), $unspec);
            else:
                byte_width = (size+7) // 8
                hex_content = split_join_text(f'{unspec}', 2, "0x", ", ")

                var2 = generate_var_name('member')

                #pre[ uint8_t $var2[${byte_width}] = { ${hex_content} };
                #pre[ if (likely(is_header_valid(HDR($hdrname), pd)))    GET_BUF($var2, dst_pkt(pd), FLD($hdrname,$fldname));
                #pre[ uint8_t* $var = $var2;
            #[ $var
    else:
        if e.type.node_type in {'Type_Enum', 'Type_Error'}:
            #[ ${e.type.members.get(e.member).c_name}
        elif e.expr('expr', lambda e2: e2.type.name == 'parsed_packet'):
            hdrname = e.expr.member
            fld = e.member
            size = hdr.urtype.fields.get(e.member).size
            unspec = unspecified_value(size)
            #pre[ check_hdr_valid_${hdrname}_${fldname}(pd, FLD($hdrname,$fldname), "$unspec");
            #[ (is_header_valid(HDR($hdrname), pd) ? pd->fields.FLD($hdrname,$fldname) : ($unspec))
        elif e.expr.node_type == 'MethodCallExpression':
            # note: this is an apply_result_t, it cannot be invalid
            #[ ${format_expr(e.expr)}.${e.member}
        elif e.expr.node_type == 'ArrayIndex':
            hdrname, fldname = get_hdrfld_name(e)
            #[ GET32(src_pkt(pd), FLD($hdrname,$fldname))
        else:
            addWarning('getting member', f'Unexpected expression of type {e.node_type}')
            #[ ${format_expr(e.expr)}.${e.member}

###########
# format extern call

def gen_list_elems(elems, last):
    if len(elems) > 0:
        elem0 = elems[0]
        #[ ${elem0}

        for elem in elems[1:]:
            #[ , ${elem}

    #[ ${', ' if len(elems) > 0 else ''}${last}


def gen_format_method_arg(par, arg, listexpr_to_buf):
    ae = arg.expression

    if ae.node_type in ('ListExpression'):
        return listexpr_to_buf[ae]

    if 'fld_ref' in ae:
        hdrname, fldname = get_hdrfld_name(ae)
        #[ (${format_type(par.urtype)})get_handle_fld(pd, FLD($hdrname, $fldname), "parameter").pointer
    else:
        fmt = format_expr(arg)
        if fmt == '':
            return None

        ref = "&" if is_ref(ae) else ""
        #[ $ref$fmt


def gen_pre_format_call_extern_make_buf_size(varsize, vardata, components):
    #pre[     int $varsize = 0
    for component in components:
        if component.node_type == 'Member':
            hdrname, fldname = get_hdrfld_name(component)
            if fldname is None:
                #pre[         + hdr_infos[HDR($hdrname)].byte_width
            else:
                hdrname, fldname = hdrname, fldname
                #pre[         + fld_infos[FLD($hdrname,$fldname)].byte_width
        elif component.node_type == 'PathExpression':
            #pre[         + (${component.urtype.size}+7)/8
        elif component.node_type == 'Cast':
            #pre[         + (${component.expr.urtype.size}+7)/8
        elif component.node_type == 'Constant':
            #pre[         + (${component.type.size}+7)/8
        else:
            addError('Calling extern', f'Encountered unexpected extern argument of type {component.node_type}')
    #pre[         ;


def gen_pre_format_call_extern_make_buf_data(component, vardata, varoffset):
    if component.node_type == 'Member':
        hdrname, fldname = get_hdrfld_name(component)
        if fldname is None:
            #pre[     memcpy($vardata + $varoffset, pd->headers[HDR($hdrname)].pointer, hdr_infos[HDR($hdrname)].byte_width);
            #pre[     $varoffset += hdr_infos[HDR($hdrname)].byte_width;
        else:
            #pre[     GET_BUF($vardata + $varoffset, dst_pkt(pd), FLD(${hdrname},$fldname));
            #pre[     $varoffset += fld_infos[FLD($hdrname,$fldname)].byte_width;
    elif component.node_type == 'PathExpression':
        name = component.path.name
        is_local = is_control_local_var(name)
        param = f'local_vars->{name}' if is_local else f'parameters.{name}'
        #pre[     memcpy($vardata + $varoffset, &($param), (${component.urtype.size}+7)/8);
        #pre[     $varoffset += (${component.urtype.size}+7)/8;
    elif component.node_type == 'Cast':
        cast = generate_var_name('cast')
        #pre[     ${format_type(component.destType, cast)} = (${format_type(component.destType)})(${format_expr(component.expr)});
        #pre[     memcpy($vardata + $varoffset, &$cast, (${component.destType.urtype.size}+7)/8);
        #pre[     $varoffset += (${component.destType.urtype.size}+7)/8;
    elif component.node_type == 'Constant':
        cast = generate_var_name('const')
        if component.urtype.size > 32:
            name, hex_content = make_const(component)
            const_var = generate_var_name(f"const{component.type.size}_", name)

            #pre[     uint8_t ${const_var}[] = {$hex_content};
            #pre[     memcpy($vardata + $varoffset, ${const_var}, (${component.type.size}+7)/8);
            #pre[     $varoffset += (${component.type.size}+7)/8;
        else:
            #pre[     *(${format_type(component.type)}*)($vardata + $varoffset) = ${format_expr(component)};
            #pre[     $varoffset += (${component.type.size}+7)/8;
    else:
        addError('Calling extern', f'Encountered unexpected extern argument of type {component.node_type}')


def gen_prepare_listexpr_arg(listexpr):
    lec = listexpr.components
    components = lec.flatmap(lambda comp: comp.components if comp.node_type == 'ListExpression' else [comp])
    hdrflds = components.map(get_hdrfld_name)

    varsize = generate_var_name('size')
    vardata = generate_var_name('buffer_data')

    gen_pre_format_call_extern_make_buf_size(varsize, vardata, components)

    varoffset = generate_var_name('offset')
    #pre[     uint8_t $vardata[$varsize];
    #pre[     int $varoffset = 0;
    for component in components:
        gen_pre_format_call_extern_make_buf_data(component, vardata, varoffset)
        #pre[

    buf = generate_var_name('buffer')
    #pre{     uint8_buffer_t $buf = {
    #pre[        .size = $varsize,
    #pre[        .buffer = $vardata,
    #pre}     };

    return buf


def gen_format_lazy_extern(args, mname, m, listexpr_to_buf):
    """A "lazy extern" is one whose first parameter is a boolean condition,
       and if it evaluates to true, the other parameters must not be evaluated,
       and the extern method must not be called.
       We also silently suppose that the extern does not return a value."""
    global compiler_common

    prebuf1 = compiler_common.pre_statement_buffer
    compiler_common.pre_statement_buffer = ""

    with SugarStyle("inline_comment"):
        pars = m.type.parameters.parameters
        fmt_args = [fmt_arg for par, arg in zip(pars, args) if not arg.is_vec() for fmt_arg in [gen_format_method_arg(par, arg, listexpr_to_buf)] if fmt_arg is not None]

    prebuf2 = compiler_common.pre_statement_buffer
    compiler_common.pre_statement_buffer = ""

    #pre[ $prebuf1
    #pre[ if (likely(${fmt_args[0]})) {
    #pre[     $prebuf2
    #[        $mname(${gen_list_elems(fmt_args, "SHORT_STDPARAMS_IN")})
    #aft[ }


def gen_format_call_extern(args, mname, m, base_mname):
    listexpr_to_buf = {le: gen_prepare_listexpr_arg(le)
                              for le in args.map('expression').filter('node_type', 'ListExpression')}

    lazy_externs = 'verify_checksum update_checksum verify_checksum_with_payload update_checksum_with_payload'.split(' ')
    if base_mname in lazy_externs:
        #= gen_format_lazy_extern(args, mname, m, listexpr_to_buf)
    else:
        with SugarStyle("inline_comment"):
            pars = m.type.parameters.parameters
            fmt_args = [fmt_arg for par, arg in zip(pars, args) if not arg.is_vec() for fmt_arg in [gen_format_method_arg(par, arg, listexpr_to_buf)] if fmt_arg is not None]

        if 'expr' in m:
            extern_name = f'EXTERNCALL0({m.expr.urtype.name},{mname})'
            xvar = m.expr.decl_ref.name

            if 'smem_type' in (smem := m.expr.decl_ref):
                smem_part = get_smem_name(smem)
                name = f'&global_smem.{smem_part}'
            else:
                name = f'&global_smem.EXTERNNAME({xvar})'

            fmt_args = [name] + fmt_args
        else:
            extern_name = mname

        #[ ${extern_name}(${gen_list_elems(fmt_args, "SHORT_STDPARAMS_IN")})

##############

def gen_fmt_MethodCallExpression(e, format_as_value=True, expand_parameters=False, needs_variable=False, funname_override=None):
    special_methods = {
        ('Member', 'setValid'):     gen_method_setValid,
        ('Member', 'isValid'):      gen_method_isValid,
        ('Member', 'setInvalid'):   gen_method_setInvalid,
        ('Member', 'apply'):        gen_method_apply,
        ('Member', 'lookahead'):    gen_method_lookahead,
    }

    m = e.method

    if 'member' in m and (m.node_type, m.member) in special_methods:
        method = special_methods.get((m.node_type, m.member))
        #= method(e)
    elif e.arguments.is_vec():
        mname = m.path.name if 'path' in m else m.member

        if mname == 'digest':
            #= gen_format_call_digest(e)
        else:
            args = e.arguments
            mname2 = funname_override or mname
            #= gen_format_call_extern(args, mname2, m, mname)
    else:
        if m('expr').type.node_type == 'Type_Extern':
            ee = m.expr
            if m.member in ('lookahead', 'advance', 'length'):
                raise NotImplementedError(f'{ee.type.name}.{m.member} is not supported yet!')

            funname = f"{ee.type.name}_t_{m.member}"
            extern_inst = format_expr(ee)
            extern_type = format_type(ee.urtype.methods.get(m.member, 'Method').urtype.returnType)
            #pre[ extern ${extern_type} ${funname}(${ee.urtype.name}_t);
            #[ $funname($extern_inst)
        else:
            funname = format_expr(m)
            #pre[ extern void ${funname}(SHORT_STDPARAMS);
            #[ $funname(SHORT_STDPARAMS_IN)

def gen_fmt_ListExpression(e, format_as_value=True, expand_parameters=False, needs_variable=False, funname_override=None):
    global generated_exprs

    if e.id not in generated_exprs:
        generated_exprs.add(e.id)

        cos = e.components.filterfalse('node_type', 'Constant')

        total_width = '+'.join((f'get_handle_fld(pd, FLD({co.expr.member},{co.fld_ref.name}), "listsize").bitwidth' for co in cos))

        #pre[ int buffer${e.id}_size = ((${total_width}) +7)/8;
        #pre[ uint8_t buffer${e.id}[buffer${e.id}_size];

        hdrs = unique_everseen(cos.map(lambda co: (co.expr.member, co.hdr_ref)))
        for hdrname, hdr in hdrs:
            field_names = e.components.filter('hdr_ref.name', hdr.name).map('member')
            idxflds = [(idx, fld) for idx, fld in enumerate(hdr.fields) if fld.name in field_names]

            offset = '0'
            while idxflds != []:
                fld0_idx, fld0 = idxflds[0]

                # note: param = (idx, (idxorig, fld))
                lam = lambda param: param[1][0] == fld0_idx + param[0]

                neighbour_flds = list(takewhile(lam, enumerate(idxflds)))
                width = '+'.join((f'get_handle_fld(pd, FLD({hdrname},{fld.name}), "fldsize").bitwidth' for _, (_, fld) in neighbour_flds))
                #pre[ memcpy(buffer${e.id} + (${offset}+7)/8, get_handle_fld(pd, FLD($hdrname, ${fld0.name}), "fldsize").pointer, ((${width}) +7)/8);

                idxflds = idxflds[len(neighbour_flds):]
                offset += f'+{width}'

    #[ (uint8_buffer_t) { .buffer = buffer${e.id}, .size = buffer${e.id}_size } /* ListExpression */

def gen_fmt_StructInitializerExpression(e, fldsvar, format_as_value=True, expand_parameters=False, needs_variable=False, funname_override=None):
    member_offset = 0
    const_offset = 0

    #{ (${gen_format_type(e.type, no_ptr=True)}) {
    for component in e.components:
        ce = component.expression
        size = ce._fld_ref.urtype.size

        if size > 32:
            if ce.node_type == 'Constant':
                name, hex_content = make_const(ce)
                #[ .${component.name} = { ${hex_content} },
            if ce.node_type == 'Member':
                # note: ${component.name}/${size//8}B is longer than 32b, will be initialised from field $hdrname.$fldname afterwards
                pass
        else:
            ctype = 'const' if ce.node_type == 'Constant' else 'meta' if 'expr' in ce and (tref := ce.expr("ref.urtype")) and tref.is_metadata else 'other'
            if ctype == 'const':
                #[ .${component.name} = ${format_expr(ce)},
            elif ctype == 'meta':
                #[ .${component.name} = (uint${align8_16_32(size)}_t)GET32(src_pkt(pd), FLD(${tref.name},${ce.member})),
            else:
                #[ .${component.name} = (uint${align8_16_32(size)}_t)${gen_format_expr(ce)},
    #} }


def gen_fmt_StructExpression(e, format_as_value=True, expand_parameters=False, needs_variable=False, funname_override=None):
    fldsvar = gen_var_name(e, 'long_fields')
    structvar = gen_var_name(e, f'struct_{e.type.name}')

    ces = e.components.map('expression')

    long_fields = ces.filter(lambda ce: ce._fld_ref.urtype.size > 32)

    const_size = sum(long_fields.filter('node_type', 'Constant').map('_hdr_ref.urtype.size'))

    #pre[ ${format_type(e.type, no_ptr=True)} $structvar = ${gen_fmt_StructInitializerExpression(e, fldsvar, format_as_value, expand_parameters, needs_variable, funname_override)};
    for c in long_fields.filter('expression.node_type', 'Member'):
        ce = c.expression
        hdrname, fldname = get_hdrfld_name(ce)
        #pre[ GET_BUF(&(${structvar}.${c.name}), src_pkt(pd), FLD($hdrname,$fldname));

    # TODO in some cases, it should be empty
    ref = '&'

    #[ $ref$structvar


def gen_fmt_Constant(e, format_as_value=True, expand_parameters=False, needs_variable=False, funname_override=None):
    if e.type.node_type == 'Type_Bits':
        name, hex_content = make_const(e)
        if e.type.size > 32:
            const_var = generate_var_name(f"const{e.type.size}_", name)

            #pre[ uint8_t ${const_var}[] = {$hex_content};
            #[ ${const_var}
        else:
            is_not_too_big = e.type.size <= 4 or e.value < 2**(e.type.size-1)
            value_hint = "" if is_not_too_big else f" /* probably -{2**e.type.size - e.value} */"
            if needs_variable:
                const_var = generate_var_name(f"const{e.type.size}_", name)
                size = align8_16_32(e.type.size)
                #pre[ uint${size}_t ${const_var} = ${with_base(e.value, e.base)}${value_hint};
                #[ ${const_var}
            else:
                #[ ${with_base(e.value, e.base)}${value_hint}
    else:
        #[ ${e.value}


def gen_fmt_Operator(e, nt, format_as_value=True, expand_parameters=False):
    unary_ops = ('Neg', 'Cmpl', 'LNot')
    if nt in unary_ops:
        fe = format_expr(e.expr)
        if nt == 'Neg':
            if e.type.node_type == 'Type_Bits' and not e.type.isSigned:
                fe2 = f'{2**e.type.size}-({fe})'
                #[ ${masking(e.type, fe2)}
            else:
                #[ (-$fe)
        elif nt == 'Cmpl':
            fe2 = f'~({fe})'
            #[ ${masking(e.type, fe2)}
        elif nt == 'LNot':
            #[ (!$fe)
    else:
        left = format_expr(e.left)
        right = format_expr(e.right)
        size = e.left.type.size
        if nt in simple_binary_ops:
            if nt == 'Equ' and size > 32:
                #[ 0 == memcmp($left, $right, (${size} + 7) / 8)
            else:
                op = simple_binary_ops[nt]
                #[ (($left) ${op} ($right))
        elif nt == 'Sub' and e.type.node_type == 'Type_Bits' and not e.type.isSigned:
            #Subtraction on unsigned values is performed by adding the negation of the second operand
            expr_to_mask = f'{left} + ({2 ** e.type.size}-{right})'
            #[ ${masking(e.type, expr_to_mask)}
        elif nt == 'Shr' and e.type.node_type == 'Type_Bits' and e.type.isSigned:
            #Right shift on signed values is performed with a shift width check
            #[ ((${right}>${size}) ? 0 : (${left} >> ${right}))
        else:
            #These formatting rules MUST follow the previous special cases
            #= gen_fmt_ComplexOp(e, complex_binary_ops[nt], format_as_value, expand_parameters)


def funs_with_cond(name_cond):
    return ((name, fun) for name, fun in getmembers(sys.modules[__name__], isfunction) if name_cond(name))

def gen_format_slice(e):
    dst_size = e.type.size
    src_size = e.e0.type.size
    src = format_expr(e.e0)

    var_src = generate_var_name('src_size')
    var_dst = generate_var_name('dst_size')
    var_end_offset = generate_var_name('end_offset')
    var_offset = generate_var_name('offset')
    #pre[ int ${var_src} = ${src_size};
    #pre[ int ${var_dst} = ${dst_size};
    #pre[ int ${var_end_offset} = ${format_expr(e.e2)};
    #pre[ int ${var_offset} = ${var_src} - (${var_end_offset} + ${var_dst});
    if dst_size <= 32:
        if src_size <= 32:
            varslice = generate_var_name('slicing')
            dsttype = format_type(e.type)
            needs_dereferencing = e.e0('needs_dereferencing', True)
            deref = '*' if needs_dereferencing else ''
            #pre[ $dsttype $varslice = ((($dsttype)($deref($src))) >> ${var_offset});
            #[ ${masking(e.type, varslice)}
        elif dst_size % 8 == 0 and src_size % 8 == 0:
            size = align8_16_32(dst_size)
            byte_size = size // 8
            endian_conversion = f"net2t4p4s_{byte_size}"

            needs_dereferencing = e.e0('needs_dereferencing', True)
            deref = "*" if needs_dereferencing else ""
            var_slice = generate_var_name(f'slice_{dst_size}b')

            typ = format_type(e.type)

            #pre[ $typ ${var_slice} = ${endian_conversion}(($deref( ($typ*)(($src) + ($var_offset/8)))));
            #[ (uint${align8_16_32(size)}_t)${var_slice}
        else:
            if dst_size % 8 != 0:
                target, size = 'destination', dst_size
            else:
                target, size = 'source', src_size
            addWarning('formatting >> operator', f'Unsupported slice: {target} size is {size}b which is not byte aligned)')
            #[ 0 /* TODO implement properly */
    else:
        addError('formatting >> operator', f'Unsupported slice: result is bigger than 32 bits ({dst_size} bits)')


def gen_format_expr(e, format_as_value=True, expand_parameters=False, needs_variable=False, funname_override=None):
    prefix = 'gen_fmt_'
    complex_cases = {name[len(prefix):]: fun for name, fun in funs_with_cond(lambda name: name.startswith(prefix))}

    if e is None:
        addError("Formatting expression", "An expression was found to be None, this should be impossible")
        return "FORMAT_EXPR(None)"

    nt = e.node_type

    if nt == 'Declaration_Instance' or nt == 'Smem_Instance':
        if e.urtype.name == 'Digest':
            # smem_name = f'EXTERNNAME(Digest{e.name})'
            # type_name = f'EXTERNTYPE(Digest{e.name})'
            smem_name = f'EXTERNNAME({e.name})'
            type_name = f'EXTERNTYPE(Digest)'
            deref = '&'
        else:
            pobs, reverse_pobs = packets_by_model(compiler_common.current_compilation['hlir'])

            # postfix = f'{pobs[e.packets_or_bytes]}' if e._smem.smem_type in ('counter', 'meter', 'direct_meter') else ''
            postfix2 = f'{e.table.name}' if 'table' in e and e.table is not None else ''
            deref = '' if e._smem.smem_type == 'register' else '&'

            name_parts = [part for part in (e._smem.smem_type, e.name, postfix2) if part != '']

            if e._smem.smem_type in ('register'):
                signed = 'int' if e.is_signed else 'uint'
                size = align8_16_32(e.size)
                type_name = f'REGTYPE({signed},{size})'
            else:
                type_name = f'SMEMTYPE({e._smem.smem_type})'

            smem_name = f'SMEM{len(name_parts)}({",".join(name_parts)})'
        #[ (${type_name}*)${deref}(global_smem.${smem_name})
    elif nt == 'Member' and 'expr' in e and 'expr' in e.expr and e.expr.expr.urtype.node_type == 'Type_Stack':
        fldname = e.member
        idx = e.expr.member
        stk = e.expr.expr.member

        if idx == 'last':
            hdr = generate_var_name(f'stack_{stk}_last')
            #pre[ header_instance_e $hdr = stk_current(STK($stk), pd);
        else:
            hdr = generate_var_name(f'stack_{stk}_{idx}')
            #pre[ header_instance_e $hdr = stk_at_idx(STK($stk), $idx, pd);

        #[ GET32(src_pkt(pd), stk_start_fld($hdr) + stkfld_offset_${stk}_$fldname)
    elif nt == 'Member' and 'fld_ref' in e:
        hdrname, fldname = get_hdrfld_name(e)

        # note: this special case is here because it uses #pre; it should be in 'gen_fmt_Member'
        if not format_as_value:
            #[ FLD(${e.expr.hdr_ref.name}, ${fldname})
        elif e.type.size > 32:
            varname = generate_var_name(f"hdr_{e.expr.hdr_ref.name}_{fldname}")
            byte_size = (e.type.size + 7) // 8

            #pre[ uint8_t $varname[${byte_size}];
            #pre[ GET_BUF($varname, src_pkt(pd), FLD($hdrname,$fldname));

            #[ $varname
        elif needs_variable:
            varname = generate_var_name(f"hdr_{e.expr.hdr_ref.name}_{fldname}")
            byte_size = (e.type.size + 7) // 8

            #pre[ ${format_type(e.type)} $varname = GET32(src_pkt(pd), FLD($hdrname,$fldname));

            #[ &$varname
        else:
            size = e.fld_ref.urtype.size
            unspec = unspecified_value(size)

            #pre{ if (!is_header_valid(HDR($hdrname), pd)) {
            #pre[     debug("   " T4LIT(!!,warning) " Access to field in invalid header " T4LIT(%s,warning) "." T4LIT(${e.member},field) ", returning \"unspecified\" value " T4LIT($unspec) "\n", hdr_infos[HDR($hdrname)].name);
            #pre} }

            #[ GET32_def(src_pkt(pd), FLD($hdrname,$fldname), $unspec)
    elif nt in complex_cases:
        case = complex_cases[nt]
        if nt == 'SelectExpression':
            with SugarStyle('line_comment'):
                #= case(e, format_as_value, expand_parameters, needs_variable, funname_override)
        else:
            #= case(e, format_as_value, expand_parameters, needs_variable, funname_override)
    elif nt in simple_binary_ops or nt in complex_binary_ops or nt in ('Neg', 'Cmpl', 'LNot'):
        #= gen_fmt_Operator(e, nt, format_as_value, expand_parameters)

    elif nt == 'DefaultExpression':
        #[ default
    elif nt == 'Parameter':
        #[ ${format_type(e.type)} ${e.name}
    elif nt == 'BoolLiteral':
        if e.value:
            #[ true
        else:
            #[ false
    elif nt == 'StringLiteral':
        varname = generate_var_name('string')
        #pre[ const char* $varname = "${e.value}";
        #[ $varname
    elif nt == 'TypeNameExpression':
        #= format_expr(e.typeName.type_ref)
    elif nt == 'Mux':
        #[ (${format_expr(e.e0)} ? ${format_expr(e.e1)} : ${format_expr(e.e2)})
    elif nt == 'Slice':
        #= gen_format_slice(e)
    elif nt == 'Concat':
        #[ ((${format_expr(e.left)} << ${e.right.type.size}) | ${format_expr(e.right)})
    elif nt == 'PathExpression':
        name = e.path.name
        is_local = is_control_local_var(name, e)
        is_abs = 'is_metadata' not in e.urtype or not e.urtype.is_metadata
        if 'action_ref' in e:
            #[ action_${e.action_ref.name}
        elif is_local:
            if e('needs_dereferencing', True):
                #[ &(local_vars->$name)
            else:
                #[ local_vars->$name
        elif is_abs:
            #[ parameters.$name
        else:
            pass
    elif nt == 'Argument':
        #= format_expr(e.expression)
    else:
        addError("formatting an expression", f"Expression {e} of type {nt} is not supported yet!")


################################################################################

method_call_envs = {}

def get_method_call_env(mcall, mname):
    global method_call_envs

    method = mcall.method.type

    # TODO maybe types other than Type_Enum will have to be included as well
    type_env_raw = {par.name: arg for par, arg in zip(method.typeParameters.parameters, mcall.typeArguments)}
    type_env_raw |= {par.type.name: par.type for par in method.parameters.parameters if par.type.node_type == 'Type_Enum' and par.type.name not in type_env_raw}

    type_env = {parname: format_type(type_env_raw[parname]) for parname in type_env_raw}

    if mname in method_call_envs and type_env != method_call_envs[mname]:
        addError("determining method call type args", f"Method {mname} is called {len(calls)} times with at least two different sets of argument types")

    return type_env


def fld_infos(e):
    for fld in e.arguments[1].expression.components:
        fe = fld.expression
        if 'expr' not in fe:
            continue
        fee = fe.expr
        if 'hdr_ref' not in fee or fee.hdr_ref.urtype.is_metadata:
            hdrname = fee.hdr_ref.name
        else:
            hdrname = fee.member
        yield hdrname, fe.member, fe.urtype.size


def make_digest_fldvars(e):
    fldvars = {}

    for hdrname, fldname, size in fld_infos(e):
        if size <= 32:
            fldtxt = f'fld_{hdrname}_{fldname}'
            fldvar = generate_var_name(fldtxt)
            fldvars[fldtxt] = fldvar

    return fldvars

def gen_print_digest_fields(e, fldvars):
    #pre[ debug("    " T4LIT(<,outgoing) " " T4LIT(Sending digest,outgoing) " to port " T4LIT(%d,port) "\n", ${e.arguments[0].expression.value});
    for hdrname, fldname, size in fld_infos(e):
        if size <= 32:
            sz = ((size+7)//8) * 8
            fldtxt = f'fld_{hdrname}_{fldname}'
            fldvar = fldvars[fldtxt]
            #pre[ uint${sz}_t $fldvar = GET32(src_pkt(pd), FLD($hdrname,$fldname));
            #pre[ dbg_bytes(&$fldvar, (${size}+7)/8, "        : "T4LIT(${hdrname},header)"."T4LIT(${fldname},field)"/"T4LIT(${size})" = ");
        else:
            #pre[ dbg_bytes(get_handle_fld(pd, FLD($hdrname,$fldname), "digestfld").pointer, (${size}+7)/8, "        : "T4LIT(${hdrname},header)"."T4LIT(${fldname},field)"/"T4LIT(${size})" = ");

    return fldvars

def gen_add_digest_fields(e, var, fldvars):
    name = e.typeArguments['Type_Name'][0].path.name

    #pre{ #ifdef T4P4S_NO_CONTROL_PLANE
    #pre[     ctrl_plane_digest $var = 0; // dummy
    #pre[ #else
    #pre[     ctrl_plane_digest $var = create_digest(bg, "$name");
    for hdrname, fldname, size in fld_infos(e):
        if size <= 32:
            sz = ((size+7)//8) * 8
            fldtxt = f'fld_{hdrname}_{fldname}'
            fldvar = fldvars[fldtxt]
            #pre[     add_digest_field($var, &$fldvar, $sz);
        else:
            #pre[     add_digest_field($var, get_handle_fld(pd, FLD($hdrname,$fldname), "digestfld").pointer, $size);
    #pre} #endif
    #pre[

def gen_format_call_digest(e):
    var = generate_var_name('digest')
    fldvars = make_digest_fldvars(e)
    gen_print_digest_fields(e, fldvars)
    gen_add_digest_fields(e, var, fldvars)

    name_postfix = "".join(e.method.type.typeParameters.parameters.filter('node_type', 'Type_Var').map('urtype.name').map(lambda n: f'__{n}'))
    funname = f'{e.method.path.name}{name_postfix}'
    receiver = e.arguments[0].expression.value

    #{ if ($var != 0) {
    #[     $funname($receiver, $var, SHORT_STDPARAMS_IN);
    #[     sleep_millis(DIGEST_SLEEP_MILLIS);
    #} }

################################################################################

def format_declaration(d, varname_override = None):
    with SugarStyle("no_comment_inline"):
        return gen_format_declaration(d, varname_override)

# TODO use the varname argument in all cases where a variable declaration is created
def format_type(t, varname = None, resolve_names = True, addon = "", no_ptr = False, type_args = [], is_atomic = False, is_const = False):
    with SugarStyle("inline_comment"):
        use_array = varname is not None
        if t.node_type == 'Type_Stack':
            use_array = True
        elif 'size' in t.urtype and t.urtype.size <= 32:
            use_array = False
        result = gen_format_type(t, resolve_names, use_array=use_array, addon=addon, no_ptr=no_ptr, type_args=type_args, is_atomic = is_atomic, is_const = is_const).strip()

    if varname is None:
        return result

    split = result.split(" ")
    essential_part = 2 if split[0] in ['enum', 'struct'] else 1

    type1 = " ".join(split[0:essential_part])
    type2 = " ".join(split[essential_part:])
    return f"{type1} {varname}{type2}"

def format_expr(e, format_as_value=True, expand_parameters=False, needs_variable=False, funname_override=None):
    with SugarStyle("inline_comment"):
        return gen_format_expr(e, format_as_value, expand_parameters, needs_variable, funname_override)

def masking(dst_type, e):
    with SugarStyle("inline_comment"):
        return gen_masking(dst_type, e)

def casting(dst_type, is_local, e):
    with SugarStyle("inline_comment"):
        return gen_casting(dst_type, is_local, e)

def format_statement(stmt, ctl=None):
    if ctl is not None:
        compiler_common.enclosing_control = ctl

    ret = gen_format_statement(stmt, ctl)

    buf1 = compiler_common.pre_statement_buffer
    buf2 = compiler_common.post_statement_buffer
    buf1 = "" if buf1 == "" else buf1.rstrip() + "\n"
    buf2 = "" if buf2 == "" else buf2.rstrip() + "\n"
    compiler_common.pre_statement_buffer = ""
    compiler_common.post_statement_buffer = ""
    return f'{buf1}{ret}{buf2}'


def format_type_mask(t):
    with SugarStyle("inline_comment"):
        return gen_format_type_mask(t)

def gen_var_name(node, prefix = None):
    if node.node_type == 'Member':
        e = node._expr
        hdrname = e.type.name if e.node_type == 'ArrayIndex' else e.member if 'member' in e else e.path.name
        #[ Member${node.id}_${hdrname}__${node.member}
    else:
        prefix = prefix or f"value_{node.node_type}"
        #[ ${prefix}_${node.id}

###########################

def get_mcall_infos(mcall):
    m = mcall.method

    if 'expr' in m:
        mname_parts = (m.expr.urtype.name, m.member)
        mtype = m.expr.urtype
        rettype = mcall.urtype
        tps = m.expr.type_parameters
    else:
        mname_parts = (m.path.name,)
        mtype = m.urtype
        rettype = mtype.returnType
        tps = mtype.typeParameters.parameters

    if mtype.node_type == 'Type_Unknown':
        return None

    ret = format_type(rettype)

    def find_index(vec_node, elem):
        return None if elem not in vec_node else vec_node.vec.index(elem)

    def make_parinfo(argexpr, par):
        parname = argexpr.path.name if argexpr.node_type == 'PathExpression' else par.name
        pardir = par.direction
        partype = par.urtype
        ctype = get_c_type(par, argexpr)
        type_par_idx = find_index(tps.map('type_ref'), par.urtype)
        return (parname, pardir, partype, ctype, type_par_idx)

    pars = m.urtype.parameters.parameters
    argexprs = mcall.arguments.map('expression')
    partypeinfos = tuple((type_par, get_partypename_ctype(type_par, None, '')[0]) for type_par in tps)
    parinfos = tuple(make_parinfo(argexpr, par) for par, argexpr in zip(pars, argexprs) if not (par.urtype.node_type == 'Type_Header' and par.urtype.is_metadata))

    return (mname_parts, parinfos, ret, partypeinfos)


def get_all_extern_call_infos(hlir):
    def is_not_smem(e):
        ut = e.method.expr.decl_ref.urtype
        return 'extern_type' not in ut or ut.extern_type != 'smem'

    methods = hlir.groups.pathexprs.under_mcall.filter('type.node_type', 'Type_Method')

    mcalls = methods.map(lambda pe: pe.parent())
    control_local_mcalls = hlir.all_nodes.by_type('MethodCallExpression').filter('method.node_type', 'Member').filter('method.expr.type.node_type', 'Type_SpecializedCanonical').filter(is_not_smem)

    all_mcalls = mcalls + control_local_mcalls

    return all_mcalls.map(get_mcall_infos)


# ------------------------------------------------------------------------------
# TODO everything below is too low level, move elsewhere (to the DPDK area)


def to_c_bool(value):
    return 'true' if value else 'false'


def get_arginfo(par, arg, ptr, is_out):
    if arg is None:
        return None, ""
    if (ee := par.urtype).node_type in ('Type_Enum', 'Type_Error'):
        return format_type(ee), f'{ee.c_name}_{arg.member}'
    if arg.node_type == 'Member':
        hdrname, fldname = get_hdrfld_name(arg)
        return f'{format_type(arg.type)} {ptr}', f'GET32(src_pkt(pd), FLD({hdrname}, {fldname}))'
    if arg.node_type == 'BoolLiteral':
        return f'bool{ptr}', par.name
    if arg.node_type == 'StringLiteral':
        return format_type(arg.type, is_const=not is_out), f'"{arg.value}"'
    if arg.node_type == 'StructExpression':
        if arg.urtype.node_type == 'Type_Struct':
            return f'{arg.urtype.name}*', f'{par.name}'
        else:
            size = '+'.join(f'{fld.urtype.size} /* TODO find the appropriate way to get size of the varwidth field */' if fld.urtype.node_type == 'Type_Varbits' else f'{fld.urtype.size}' for fld in par.urtype.fields)
            return 'uint8_buffer_t', f'to_uint8_buffer_t({size}, {par.name})'
    if arg.node_type == 'Constant':
        # note: this is not really a constant, just a dummy
        return format_type(arg.type), par.name
    if arg.node_type == 'PathExpression':
        return format_type(arg.type), f'{arg.path.name}'
    if arg.node_type == 'MethodCallExpression':
        arg_mname = arg.method.member
        if arg_mname == 'isValid' and len(arg.arguments) == 0:
            hdrname = arg.method.expr.member
            return 'bool', f'is_header_valid(HDR({hdrname}), pd)';
        else:
            callargs = ', '.join(format_expr(argexpr) for argexpr in arg.arguments)
            return format_type(arg.method.returnType), f'{arg_mname}({callargs})'

    addError('Determining C type', f'Unknown P4 node type {par.node_type}')
    return None, None

def get_partypename_ctype(par, arg, ptr):
    purtype = par.urtype

    if purtype.node_type == 'Type_Boolean':
        return None, f'bool{ptr}'
    if (ee := par.urtype).node_type in ('Type_Enum', 'Type_Error'):
        return f'{arg.member}', f'{format_type(par.urtype)}'
    if (bits := purtype).node_type == 'Type_Bits':
        # TODO this case needs further consideration
        if bits.size > 32:
            return 'buf', f'uint8_t*{ptr}'

        size = align8_16_32(bits.size)
        return f'u{size}', f'uint{size}_t{ptr}'
    if (struct := purtype).node_type == 'Type_Struct':
        return f'{struct.name}', f'{append_type_postfix(struct.name)}*{ptr}'
    if (param := purtype).node_type == 'Parameter':
        size = align8_16_32(bits.size)
        return None, f'uint{size}_t{ptr}'
    if (string := purtype).node_type == 'Type_String':
        return None, f'const char*'

    return None, None

def get_c_type(par, arg):
    purtype = par.urtype

    if 'is_metadata' in purtype and purtype.is_metadata:
        return None, None, None, None

    is_out = par.direction in ('out', 'inout')
    ptr = f'* /* {par.direction} */ ' if is_out else ''

    argtype, argvalue = get_arginfo(par, arg, ptr, is_out)
    partypename, ctype = get_partypename_ctype(par, arg, ptr)

    return partypename, ctype, argtype, argvalue
