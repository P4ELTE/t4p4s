# SPDX-License-Identifier: Apache-2.0
# Copyright 2017 Eotvos Lorand University, Budapest, Hungary

from inspect import getmembers, isfunction
import sys

from compiler_log_warnings_errors import addWarning, addError
from compiler_common import types, with_base, resolve_reference, is_subsequent, groupby, group_references, fldid, fldid2, pp_type_16, make_const, SugarStyle, prepend_statement, append_statement, is_control_local_var, generate_var_name, pre_statement_buffer, post_statement_buffer, enclosing_control, unique_everseen, unspecified_value, get_raw_hdr_name, get_hdr_name, get_hdrfld_name, split_join_text
from hlir16.hlir_attrs import simple_binary_ops, complex_binary_ops
from itertools import takewhile

################################################################################

# note: some functions with well-known names in C have to be renamed
funname_map = {
    "random": "random_fun",
}

################################################################################

def sizeup(size, use_array):
    return 8 if use_array else 8 if size <= 8 else 16 if size <= 16 else 32 if size <= 32 else 8


def type_to_str(t):
    if t.node_type == 'Type_Bits':
        sign = 'int' if t.isSigned else 'uint'
        return f'{sign}{t.size}_t'
    return f'{t.name}_t'

def append_type_postfix(name):
    return name if name.endswith('_t') else f'{name}_t'

def gen_format_type(t, resolve_names = True, use_array = False, addon = ""):
    """Returns a type. If the type has a part that has to come after the variable name in a declaration,
    such as [20] in uint8_t varname[20], it should be separated with a space."""

    if t.node_type == 'Type_Void':
        #[ void
    elif t.node_type == 'Type_Boolean':
        #[ bool
    elif t.node_type == 'Type_Error':
        name = t.c_name if 'c_name' in t else t.name
        #[ ${name}_t
    elif t.node_type == 'Type_List':
        #[ uint8_buffer_t
    elif t.node_type == 'Type_String':
        #[ char*
    elif t.node_type == 'Type_Varbits':
        #[ uint8_t [${(t.size+7)//8}] /* preliminary type for varbits */
    elif t.node_type == 'Type_Struct':
        base_name = re.sub(r'_t$', '', t.name)
        #[ ${base_name}_t*
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
                constructor = t.urtype.methods.get(t.urtype.name)
                datatype = constructor.type.parameters.parameters[0].urtype
                #= format_type(datatype)
            else:
                int_type = 'int' if t.arguments[0].isSigned else 'uint'
                bitsize = t.arguments[0].size
                size = sizeup(bitsize, use_array)

                #[ ${t.baseType.path.name}_${int_type}${size}_t
    elif t.node_type == 'Type_Bits':
        size = sizeup(t.size, use_array)
        base = f'int{size}_t' if t.isSigned else f'uint{size}_t'
        if use_array:
            #[ $base $addon[(${t.size} + 7) / 8]
        else:
            ptr = '*' if t.size > 32 else ''
            #[ $base$ptr $addon
    elif t.node_type == 'Type_Enum':
        name = t.c_name if 'c_name' in t else t.name
        name = f'enum_{name}' if not name.startswith('enum_') else name
        #[ ${append_type_postfix(name)}
    elif t.node_type == 'Type_Name':
        t2 = t.urtype
        if not resolve_names and 'name' in t2:
            #[ ${t2.name}
        else:
            #= gen_format_type(t2, resolve_names, use_array, addon)
    elif t.node_type == 'Type_Extern':
        is_smem = 'smem_type' in t or t.name in ('register')
        if not is_smem:
            #[ ${t.name}_t
        else:
            params = t.typeParameters.parameters

            postfix = ''.join(f'_{type_to_str(par.urtype)}' for par in params if par is not None if par.urtype is not None)
            postfix = '_t' if postfix == '' else postfix

            #[ ${t.name}${postfix}
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
    if t.size <= 8:
        return "8"
    if t.size <= 16:
        return "16"
    if t.size <= 32:
        return "32"
    return "bytebuf"

def member_to_hdr_fld_info(member_expr):
    if member_expr.urtype.node_type == 'Type_Varbits' and 'member' not in member_expr.expr:
        return member_expr.expr.path.name, member_expr.member

    hdrname = "all_metadatas" if member_expr.expr.hdr_ref('is_metadata', False) else member_expr.expr.member if 'member' in member_expr.expr else member_expr.member
    if 'member' not in member_expr.expr:
        return hdrname, None
    return hdrname, member_expr.member


def member_to_hdr_fld(member_expr):
    hdrname, fldname = member_to_hdr_fld_info(member_expr)
    # assert fldname is not None, f"Expression {member_expr} refers to header {hdrname} but no field reference found"
    if fldname is None:
        return f'HDR({hdrname})', None
    return f'HDR({hdrname})', f'FLD({hdrname},{fldname})'

def member_to_fld_name(member_expr):
    hdrname, fldname = member_to_hdr_fld(member_expr)
    return fldname


def gen_format_statement_fieldref_wide(dst, src, dst_width, dst_is_vw, dst_bytewidth, dst_name, dst_hdrname, dst_fld_name):
    if src.node_type == 'Member':
        src_pointer = generate_var_name('tmp_fldref')
        #[ uint8_t $src_pointer[$dst_bytewidth];

        if 'fld_ref' in src:
            hdrinst = 'all_metadatas' if src.expr.urtype.is_metadata else src.expr.member
            #[ EXTRACT_BYTEBUF_PACKET(pd, HDR(${hdrinst}), ${member_to_fld_name(src)}, ${src_pointer});
            if dst_is_vw:
                src_vw_bitwidth = f'pd->headers[HDR({src.expr.member})].var_width_field_bitwidth'
                dst_bytewidth = f'({src_vw_bitwidth}/8)'
        else:
            srcname = src.expr.hdr_ref.name
            hdrname, fldname = get_hdrfld_name(src)

            if hdrname is not None and fldname is not None:
                #[ EXTRACT_BYTEBUF_PACKET(pd, HDR($hdrname), FLD($hdrname,$fldname), $src_pointer);
            else:
                #[ EXTRACT_BYTEBUF_BUFFER(pstate->${srcname}, pstate->${srcname}_var, ${member_to_fld_name(src)}, $src_pointer);

            if dst_is_vw:
                src_vw_bitwidth = f'pd->headers[HDR({srcname})].var_width_field_bitwidth'
                dst_bytewidth = f'({src_vw_bitwidth}/8)'
    elif src.node_type == 'PathExpression':
        name = src.path.name
        src_pointer = f'local_vars->{name}' if is_control_local_var(name) else f'parameters.{name}'
    elif src.node_type == 'Constant':
        src_pointer = generate_var_name('tmp_fldref_const')
        #[ uint8_t $src_pointer[$dst_bytewidth] = ${int_to_big_endian_byte_array_with_length(src.value, dst_bytewidth, src.base)};
    elif src.node_type == 'Mux':
        src_pointer = generate_var_name('tmp_fldref_mux')
        #[ uint8_t $src_pointer[$dst_bytewidth] = ((${format_expr(src.e0.left)}) == (${format_expr(src.e0.right)})) ? (${format_expr(src.e1)}) : (${format_expr(src.e2)});
    elif src.node_type in simple_binary_ops and src.right.node_type == 'Constant':
        binop = src.node_type
        src_pointer = generate_var_name(f'tmp_binop_{binop}')
        const = generate_var_name(f'tmp_const_0x{src.right.value:02x}')
        #[ uint8_t $src_pointer[$dst_bytewidth];
        #[ uint8_t $const[$dst_bytewidth] = ${int_to_big_endian_byte_array_with_length(src.right.value, dst_bytewidth, src.right.base)};
        #[ for (int i = 0; i < ${dst_bytewidth}; ++i)    $src_pointer[i] ${simple_binary_ops[binop]}= $const[i];
    else:
        src_pointer = 'NOT_SUPPORTED'
        addError('formatting statement', f'Assignment to unsupported field in: {format_expr(dst)} = {src}')

    dst_fixed_size = dst.expr.hdr_ref.urtype.size - dst.fld_ref.size

    if dst_is_vw:
        #[ pd->headers[$dst_hdrname].var_width_field_bitwidth = ${src_vw_bitwidth};
        #[ pd->headers[$dst_hdrname].length = ($dst_fixed_size + pd->headers[$dst_hdrname].var_width_field_bitwidth)/8;

    #[ MODIFY_BYTEBUF_BYTEBUF_PACKET(pd, $dst_hdrname, $dst_fld_name, $src_pointer, $dst_bytewidth);

    #[ dbg_bytes($src_pointer, $dst_bytewidth,
    #[      "    " T4LIT(=,field) " Set " T4LIT(%s,header) "." T4LIT(%s,field) "/" T4LIT(%db) " (" T4LIT(%d) "B) = ",
    #[      header_instance_names[$dst_hdrname],
    #[      field_names[$dst_fld_name], // $dst_fld_name
    #[      $dst_bytewidth*8,
    #[      $dst_bytewidth
    #[      );

def is_primitive(typenode):
    """Returns true if the argument node is compiled to a non-reference C type."""
    # TODO determine better if the source is a reference or not
    return typenode.node_type == "Type_Boolean" or (typenode.node_type == 'Type_Bits' and typenode.size <= 32)


def gen_format_statement_fieldref_short(dst, src, dst_width, dst_is_vw, dst_bytewidth, dst_name, dst_hdrname, dst_fld_name):
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
    #[     set_field((fldT[]){{pd, $dst_hdrname, $dst_fld_name}}, 0, $varname, $dst_width);
    #[ } else {
    #[     debug("   " T4LIT(!!,warning) " Ignoring assignment to field in invalid header: " T4LIT(%s,warning) "." T4LIT(%s,field) "\n", hdr_infos[$dst_hdrname].name, field_names[$dst_fld_name]);
    #} }


def gen_format_statement_fieldref(dst, src):
    #TODO: handle preparsed fields, width assignment for vw fields and assignment to buffer instead header fields
    dst_width = dst.type.size
    dst_is_vw = dst.type.node_type == 'Type_Varbits'
    dst_bytewidth = (dst_width+7)//8

    assert(dst_width == src.type.size)
    assert(dst_is_vw == (src.type.node_type == 'Type_Varbits'))

    dst_name = dst.expr.member if dst.expr.node_type == 'Member' else dst.expr.path.name if dst.expr('hdr_ref', lambda h: h.urtype.is_metadata) else dst.expr._hdr_ref._path.name
    dst_hdrname, dst_fld_name = member_to_hdr_fld(dst)

    if dst_width <= 32:
        #= gen_format_statement_fieldref_short(dst, src, dst_width, dst_is_vw, dst_bytewidth, dst_name, dst_hdrname, dst_fld_name)
    else:
        #= gen_format_statement_fieldref_wide(dst, src, dst_width, dst_is_vw, dst_bytewidth, dst_name, dst_hdrname, dst_fld_name)


def is_atomic_block(blckstmt):
    try:
        return any(blckstmt.annotations.annotations.filter('name', 'atomic'))
    except:
        return False


def gen_do_assignment(dst, src):
    if dst.type.node_type == 'Type_Header':
        src_hdrname = src.path.name if src.node_type == 'PathExpression' else src.member
        dst_hdrname = dst.path.name if dst.node_type == 'PathExpression' else dst.member

        #[ do_assignment(HDR(${dst_hdrname}), HDR(${src_hdrname}), SHORT_STDPARAMS_IN);
    elif dst.type.node_type == 'Type_Bits':
        # TODO refine the condition to find out whether to use an assignment or memcpy
        requires_memcpy = src.type.size > 32 or 'decl_ref' in dst
        # is_assignable = src.type.size in [8, 32]
        is_assignable = src.type.size <= 32

        if src.type.node_type == 'Type_Bits' and not requires_memcpy:
            if is_assignable:
                if dst("expr.hdr_ref.urtype.is_metadata") or dst("hdr_ref.urtype.is_metadata"):
                    fldname = dst.member if dst("expr.hdr_ref.urtype.is_metadata") else dst.field_name
                    #[ set_field((fldT[]){{pd, HDR(all_metadatas), FLD(all_metadatas,$fldname)}}, 0, ${format_expr(src, expand_parameters=True)}, ${dst.urtype.size});
                elif dst.node_type == 'Slice':
                    #[ // TODO assignment to slice
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
                            #[ MODIFY_INT32_INT32_BITS_PACKET(pd, HDR($hdrname), FLD($hdrname,$fldname), $srcbuf);
                        else:
                            pass

                        #[ debug("       : " T4LIT($hdrname,header) "." T4LIT($fldname,field) " = " T4LIT(%d,bytes) " (" T4LIT(%${(src.type.size+7)//8}x,bytes) ")\n", ${format_expr(dst)}, ${format_expr(dst)});
                    else:
                        #[ debug("       : " T4LIT(${format_expr(dst)},header) " = " T4LIT(%d,bytes) " (" T4LIT(%${(src.type.size+7)//8}x,bytes) ")\n", ${format_expr(dst)}, ${format_expr(dst)});

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
                        #[ MODIFY_BYTEBUF_BYTEBUF_PACKET(pd, HDR(${hdr.name}), FLD(${hdr.name},${fldname}), &$tmpvar, sizeof(${format_type(dst.type)}));
                        #[ dbg_bytes(get_fld_pointer(pd, FLD(${hdr.name},${fldname})), sizeof(${format_type(dst.type)}), "        : "T4LIT(${hdr.name},header)"."T4LIT(${fldname},field)"/"T4LIT(%zuB)" = ", sizeof(${format_type(dst.type)}));
        else:
            srcexpr = format_expr(src, expand_parameters=True, needs_variable=True)
            with SugarStyle('no_comment'):
                dsttxt = gen_format_expr(dst).strip()
                srctxt = gen_format_expr(src, expand_parameters=True).strip()

            size = (dst.type.size+7)//8
            pad_size = 4 if size == 3 else size
            net2t4p4s = '' if size > 4 else f'net2t4p4s_{pad_size}'

            tmpvar = generate_var_name('assignment')

            if dst.node_type == 'Member':
                hdrname = dst.expr.hdr_ref.name
                fldname = dst.member
                #[ ${format_type(dst.type)} $tmpvar = $net2t4p4s((${format_type(dst.type)})(${format_expr(src, expand_parameters=True, needs_variable=True)}));
                #[ dbg_bytes(&($srcexpr), $size, "    : Set " T4LIT(%s,header) "." T4LIT(%s,field) "/" T4LIT(%dB) " = " T4LIT(%s,header) " = ", "$hdrname", "$fldname", $size, "$srctxt");
                #[ MODIFY_BYTEBUF_BYTEBUF_PACKET(pd, HDR($hdrname), FLD($hdrname,$fldname), &$tmpvar, $size);
            else:
                deref = "*" if src.node_type in ("Constant", "Member") and size <= 4 else ""
                #[ ${format_type(dst.type)} $tmpvar = $deref(${format_type(dst.type)}$deref)((${format_expr(src, expand_parameters=True, needs_variable=True)}));
                #[ memcpy(&(${format_expr(dst)}), &($tmpvar), ${pad_size});
                #[ dbg_bytes(&(${format_expr(dst)}), $size, "    : Set " T4LIT(%s,header) "/" T4LIT(%dB) " = ", "$dsttxt", $size);
    elif dst.node_type == 'Member':
        tmpvar = generate_var_name('assign_member')
        hdrname = dst.expr.hdr_ref.name
        fldname = dst.member

        #pre[ ${format_type(dst.type)} $tmpvar = ${format_expr(src, expand_parameters=True)};
        #[ MODIFY_INT32_BYTEBUF_PACKET(pd, HDR($hdrname), FLD($hdrname,$fldname), &$tmpvar, sizeof(${format_type(dst.type)}));
    else:
        #[ ${format_expr(dst)} = ${format_expr(src, expand_parameters=True)};


def is_extract(m):
    return m.member == 'extract' and len(m.type.parameters.parameters) == 1 and m.expr.type.name == 'packet_in'

def gen_format_statement(stmt):
    if stmt.node_type == 'AssignmentStatement':
        dst = stmt.left
        src = stmt.right
        if 'fld_ref' in dst:
            #= gen_format_statement_fieldref(dst, src)
        else:
            #= gen_do_assignment(dst, src)
    elif stmt.node_type == 'BlockStatement':
        is_atomic = is_atomic_block(stmt)
        if is_atomic:
            #[ LOCK(&${stmt.enclosing_control.type.name}_lock)
        for c in stmt.components:
            #= gen_format_statement(c)
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
        #=     format_statement(stmt.ifTrue)
        if 'ifFalse' in stmt:
            #[ } else {
            #=     format_statement(stmt.ifFalse)
        #} }
        #[ ${post}
    elif stmt.node_type == 'MethodCallStatement':
        mcall = stmt.methodCall
        m = mcall.method

        if 'member' in m and is_extract(m):
            hdrname = stmt.methodCall.arguments[0].expression.hdr_ref.name
            vw = generate_var_name('vw')

            result_len = generate_var_name(f'extracted_hdr_len__{hdrname}')

            #pre[ int parser_extract_$hdrname(uint32_t, STDPARAMS);
            #pre[ int $vw = hdr_infos[HDR(${hdrname})].var_width_field;
            #[ int ${result_len} = parser_extract_$hdrname($vw == FIXED_WIDTH_FIELD ? 0 : fld_infos[$vw].byte_width * 8, STDPARAMS_IN);

            # TODO activate or remove
            # { if (unlikely(${result_len} < 0)) {
            # [     drop_packet(STDPARAMS_IN);
            # [     return;
            # } }
        elif m.node_type == 'Method' and m.name == 'digest':
            #= gen_format_methodcall_digest(m, mcall)
        elif 'member' in m and not is_extract(m):
            #= gen_fmt_methodcall(mcall, m)
        elif 'expr' in m and m.expr.urtype.node_type == 'Type_Stack':
            #[ /* TODO stack */
        else:
            #= gen_methodcall(mcall)
    elif stmt.node_type == 'SwitchStatement':
        #[ switch(${format_expr(stmt.expression)}) {
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
    #[ fields.field_offsets = malloc(sizeof(uint8_t*)*fields.fields_quantity);
    #[ fields.field_widths = malloc(sizeof(uint8_t*)*fields.fields_quantity);

    for idx, f in enumerate(fields.components):
        if f.expr.type.is_metadata:
            #[ fields.field_offsets[$idx] = (uint8_t*) get_fld_pointer(pd, FLD(${f.expr.name},${f.member}));
            #[ fields.field_widths[$idx]  =            fld_infos[FLD(${f.expr.name},${f.member})].bit_width;
        else:
            #[ fields.field_offsets[$idx] = (uint8_t*) get_fld_pointer(pd, FLD(${f.expr.member},${f.expression.fld_ref.name}));
            #[ fields.field_widths[$idx]  =            fld_infos[FLD(${f.expr.member},${f.expression.fld_ref.name})].bit_width;
    #[ generate_digest(bg,"${digest_name}",0,&fields);
    #[ sleep_millis(DIGEST_SLEEP_MILLIS);

def is_emit(m):
    return m.expr._ref.urtype('name', lambda n: n == 'packet_out')

def gen_isValid(hdrname):
    #[ controlLocal_tmp_0 = (pd->headers[HDR($hdrname)].pointer != NULL);

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

    #[ pd->header_reorder[pd->emit_hdrinst_count] = HDR($hdrname);
    #[ ++pd->emit_hdrinst_count;

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
    elif 'table_ref' in m.expr and m.member == 'apply':
        #[ ${gen_method_apply(mcall)};
    elif m.member in specials:
        hdrname, mname = get_hdrfld_name(m)
        #= gen_${mname}(hdrname)
    elif 'member' not in m.expr:
        #= gen_fmt_methodcall_extern(m, mcall)
    elif 'expr' in m and m.expr.urtype.node_type == 'Type_Stack':
        stk, mname = get_hdrfld_name(m)
        #= gen_stk_${mname}(stk, mcall)
    else:
        hdrname = m.expr.member
        #= gen_methodcall(mcall)

def gen_fmt_methodcall_extern(m, mcall):
    # TODO treat smems and digests separately
    # currently, for legacy reasons, smem_type can take the value 'Digest'
    dref = m.expr.decl_ref
    smem_type = dref.smem_type if 'decl_ref' in m.expr and 'smem_type' in dref else m.expr.urtype.name

    # if the extern is about both packets and bytes, it takes two separate calls
    is_possibly_multiple = smem_type in ("counter", "meter", "direct_counter", "direct_meter", "Counter")
    if is_possibly_multiple:
        if dref.packets_or_bytes == "packets_and_bytes":
            #= gen_format_extern_single(m, mcall, smem_type, is_possibly_multiple, "packets")
            #= gen_format_extern_single(m, mcall, smem_type, is_possibly_multiple, "bytes")
        else:
            #= gen_format_extern_single(m, mcall, smem_type, is_possibly_multiple, dref.packets_or_bytes)
    else:
        #= gen_format_extern_single(m, mcall, smem_type, is_possibly_multiple)


def gen_smem_expr(smem, packets_or_bytes):
    if smem.type.node_type == 'Type_Specialized':
        #= format_expr(smem)
    else:
        smem_name = smem.urtype.name
        prefix = '' if smem_name == 'Digest' else f'{smem_name}_'
        pob = packets_or_bytes or ('packets_or_bytes' in smem and smem.packets_or_bytes)
        postfix = f'_{pob}' if smem_name in ('counter', 'meter', 'direct_counter') else ''
        postfix2 = f'_{smem.table.name}' if 'table' in smem else ''
        # TODO improve this for PSA smems
        is_direct = 'is_direct' in smem and smem.is_direct
        postfix3 = '[0]' if not is_direct else ''
        #[ &(global_smem.${prefix}${smem.name}${postfix}${postfix2}${postfix3})

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

def gen_extern_call_problem_type(m, mcall, smem_type, is_possibly_multiple, dref, args, packets_or_bytes = None):
    no_msg_all_ok = 0
    msg_idx_too_high = 1

    amount_idxs = {
        ('register', 'read'): 2,
        ('register', 'write'): 1,
    }

    smem_info = (smem_type, m.member)
    if smem_info in amount_idxs:
        amount_idx = amount_idxs[smem_info]
        arg, var, is_ref = args[amount_idx]
        amount_type = format_type(arg.expression.type)
        #[ ((${amount_type})(${dref.amount}) /* max. amount */ < ${var} /* actual amount */ ? ${msg_idx_too_high} : ${no_msg_all_ok})
    else:
        #[ ${no_msg_all_ok}

def get_short_type(n):
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
    from hlir16.p4node import P4Node
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
        #[ pd->is_emit_reordering = true;
        #[ debug("   :: " T4LIT(Advancing packet,status) " by " T4LIT(${size}b) " (" T4LIT(${(size+7)//8}B) ")\n");
    else:
        dref = mcall.method.expr.decl_ref

        args = [(arg, generate_var_name('declarg'), False) for arg in dref.arguments] + [(arg, generate_var_name(f'arg_{par.name}'), par.direction == 'out') for arg, par in zip(mcall.arguments, mcall.method.type.parameters.parameters)]

        tuples = ((var, format_type_ref(arg, is_ref)) for arg, var, is_ref in args)
        funargs, argtypes = zip(*tuples)

        if packets_or_bytes is not None:
            funargs = replace_pob(m, funargs, packets_or_bytes)

        funargs = ", ".join(list(funargs) + [gen_smem_expr(dref, packets_or_bytes), 'SHORT_STDPARAMS_IN'])
        argtypes = ", ".join(list(argtypes) + [f'{format_type(dref.urtype)}*', 'SHORT_STDPARAMS'])

        type_args_postfix = "".join(get_mcall_type_args(mcall).map('urtype').map(get_short_type).map(lambda n: f'__{n}'))

        funname = f'extern_{extern_name}_{m.member}{type_args_postfix}'

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

        vars_txt = ''.join(f', {var}' for arg, var, is_ref in args)

        dollar = '$'
        #[ int $problem = ${problem_code};
        #{ if (likely($problem == 0)) {
        #[     $funname($funargs);
        #[ } else {
        #{     #ifdef T4P4S_DEBUG
        #{         const char* cause_fmt[] = {
        #[             "(no problem)",
        #[             "register index (" T4LIT(%2${dollar}d = 0x%2${dollar}x) ") is too high",
        #}         };
        #[         char cause_txt[256];
        #[         sprintf(cause_txt, cause_fmt[$problem] ${vars_txt});
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
            #[ MODIFY_INT32_INT32_BITS_PACKET(pd, HDR($hdrname), FLD($hdrname,$fldname), *$var);


def is_ref(node):
    not_of_type   = node.node_type not in ('Constant', 'BoolLiteral', 'MethodCallExpression')
    not_of_urtype = node.urtype.node_type not in ('Type_Error', 'Type_Enum', 'Type_List')
    return not_of_type and not_of_urtype


def gen_methodcall(mcall):
    m = mcall.method

    def format_with_ref(e):
        ut = e.urtype
        if ut.node_type in ('Type_List'):
            return 'uint8_buffer_t'
        if ut.node_type in ('Type_Header'):
            return 'bitfield_handle_t' if not ut.is_metadata else None
        ref = "*" if is_ref(e) else ""
        return f'{format_type(ut)}{ref}'

    type_args_postfix = "".join(get_mcall_type_args(mcall).map('urtype').map(get_short_type).map(lambda n: f'__{n}'))

    funname = f'{m.path.name}{type_args_postfix}'

    argtypes = ", ".join(mcall.arguments.filter(lambda arg: not arg.is_vec()).map('expression').map(format_with_ref).filter(lambda t: t is not None) + ['SHORT_STDPARAMS'])
    funname = funname_map[funname] if funname in funname_map else funname

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
    #[ (pd->headers[${gen_method_hdr_ref(e)}].pointer != NULL)

def gen_method_setInvalid(e):
    #[ pd->headers[${gen_method_hdr_ref(e)}].pointer = NULL

def gen_method_apply(e):
    action = e.method.expr.path.name
    #[ ${action}_apply(STDPARAMS_IN)

def gen_method_lookahead(e):
    var = generate_var_name('lookahead')

    arg0 = e.typeArguments[0]
    size = arg0.size

    if size > 32:
        addError('doing lookahead', f'Lookahead was called on a type that is {size} bits long; maximum supported length is 32')

    byte_size = (size+7) // 8
    if byte_size == 3:
        byte_size = 4

    #pre[ ${gen_format_type(arg0)} $var = net2t4p4s_${byte_size}(topbits_${byte_size}(t4p4s2net_${byte_size}(*(${gen_format_type(arg0)}*)(pd->extract_ptr)), ${size}));
    #[ $var

def gen_method_setValid(e):
    hdr = e.method.expr.hdr_ref

    # TODO fix: f must always have an is_vw attribute
    def is_vw(fld):
        if fld.get_attr('is_vw') is None:
            return False
        return f.is_vw

    # TODO is this the max size?
    length = (sum(fld.size if not is_vw(f) else 0 for fld in h.urtype.fields) + 7) // 8

    #[ pd->headers[${hdr.name}] = (header_descriptor_t) {
    #[     .type = ${hdr.name},
    #[     .length = $length,
    #[     .pointer = calloc(${hdr.urtype.byte_width}, sizeof(uint8_t)),
    #[     /*TODO determine and set this field*/
    #[     .var_width_field_bitwidth = 0,
    #[ };


def gen_casting(dst_type, is_local, expr_str):
    dt   = format_type(dst_type)
    varname = generate_var_name('casting')
    if is_local:
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

def gen_fmt_ComplexOp(e, op, format_as_value=True, expand_parameters=False):
    et = e.type
    op_expr = f'({format_expr(e.left)}{op}{format_expr(e.right)})'
    if e.type.node_type == 'Type_InfInt':
        #= op_expr
    elif e.type.node_type == 'Type_Bits':
        if not e.type.isSigned:
            #[ ${masking(e.type, op_expr)}
        elif e.type.size in {8,16,32}:
            #[ ((${format_type(e.type)})${op_expr})
        else:
            addError('formatting an expression', f'Expression of type {e.node_type} is not supported on int<{e.type.size}>. (Only int<8>, int<16> and int<32> are supported.)')
            #[ ERROR

def get_select_conds(select_expr, case):
    cases_tmp = case.keyset.components if case.keyset.node_type == 'ListExpression' else [case.keyset]

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
            if case_type == 'Range':
                conds.append('{0} <= {1} && {1} <= {2}'.format(format_expr(c.left), gen_var_name(k), format_expr(c.right)))
            elif case_type == 'Mask':
                conds.append('({0} & {1}) == ({2} & {1})'.format(format_expr(c.left), format_expr(c.right), gen_var_name(k)))
            else:
                if case_type not in {'Constant'}: #Trusted expressions
                    addWarning('formatting a select case', f'Select statement cases of type {case_type} on {pp_type_16(k.type)} might not work properly.')
                conds.append(f'{gen_var_name(k)} == {format_expr(c)}')
        else:
            addError('formatting a select case', f'Select statement cases of type {case_type} on {pp_type_16(k.type)} is not supported!')
    return ' && '.join(conds), pres

def gen_fmt_SelectExpression(e, format_as_value=True, expand_parameters=False, needs_variable=False, funname_override=None):
    #Generate local variables for select values
    for k in e.select.components:
        varname = gen_var_name(k)
        if k.type.node_type == 'Type_Bits' and k.type.size <= 32:
            deref = "" if 'path' not in k else "*" if is_control_local_var(k.path.name, start_node=k) else ""
            #pre[ ${format_type(k.type)} $varname = $deref(${format_expr(k)});
        elif k.type.node_type == 'Type_Bits' and k.type.size % 8 == 0:
            #pre[ uint8_t $varname[${k.type.size/8}];
            #pre[ EXTRACT_BYTEBUF_PACKET(pd, ${format_expr(k, False)}, $varname);'
        else:
            addError('formatting select expression', f'Select on type {pp_type_16(k.type)} is not supported!')

    if len(e.selectCases) == 0:
        #[ // found a select expression with no cases, no code to generate
        return

    conds_pres = [get_select_conds(e, case) for idx, case in enumerate(e.selectCases)]

    for _, pres in conds_pres:
        for pre in pres:
            #pre[ ${pre}

    for idx, ((cond, _pre), case) in enumerate(zip(conds_pres, e.selectCases)):
        if idx == 0:
            #{ if (${cond}) { // select case #${idx+1}
        else:
            #[ } else if (${cond}) { // select case #${idx+1}
        #[     parser_state_${case.state.path.name}(STDPARAMS_IN);
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
            size = hdr.urtype.fields.get(e.member).size
            unspec = unspecified_value(size)
            #pre{ if (!is_header_valid(HDR(${hdr.name}), pd)) {
            #pre[     debug("   " T4LIT(!!,warning) " Access to field in invalid header " T4LIT(${hdr.name},warning) "." T4LIT(${e.member},field) ", returning \"unspecified\" value " T4LIT($unspec) "\n");
            #pre} }
            #[ (is_header_valid(HDR(${hdr.name}), pd) ? GET_INT32_AUTO_PACKET(pd, HDR(${hdr.name}), FLD(${hdr.name},${e.member})) : ($unspec))
        else:
            is_meta = hdr.urtype('is_metadata', False)
            hdrname = "all_metadatas" if is_meta else hdr.name
            fld = hdr.urtype.fields.get(e.member)
            fldname = e.member
            fldtype = fld.orig_fld.type
            size = fld.size
            unspec = 0 if is_meta else unspecified_value(size)
            var = generate_var_name('member')

            #pre{ if (!is_header_valid(HDR($hdrname), pd)) {
            #pre[     debug("   " T4LIT(!!,warning) " Access to field in invalid header " T4LIT($hdrname,warning) "." T4LIT(${e.member},field) ", returning \"unspecified\" value " T4LIT($unspec) "\n");
            #pre} }
            if size <= 32:
                #pre[ ${format_type(fldtype)} $var = is_header_valid(HDR($hdrname), pd) ? GET_INT32_AUTO_PACKET(pd, HDR($hdrname), FLD($hdrname,$fldname)) : ($unspec);
            else:
                byte_width = (size+7) // 8
                hex_content = split_join_text(f'{unspec}', 2, "0x", ", ")

                var2 = generate_var_name('member')

                #pre[ uint8_t $var2[${byte_width}] = { ${hex_content} };
                #pre[ if (is_header_valid(HDR($hdrname), pd))    EXTRACT_BYTEBUF_PACKET(pd, HDR($hdrname), FLD($hdrname,$fldname), $var2);
                #pre[ uint8_t* $var = $var2;
            #[ $var
    else:
        if e.type.node_type in {'Type_Enum', 'Type_Error'}:
            #[ ${e.type.members.get(e.member).c_name}
        elif e.expr('expr', lambda e2: e2.type.name == 'parsed_packet'):
            hdr = e.expr.member
            fld = e.member
            size = hdr.urtype.fields.get(e.member).size
            unspec = unspecified_value(size)
            #pre{ if (!is_header_valid(HDR($hdrname), pd)) {
            #pre[     debug("   " T4LIT(!!,warning) " Access to field in invalid header " T4LIT($hdrname,warning) "." T4LIT($fld,field) ", returning \"unspecified\" value " T4LIT($unspec) "\n");
            #pre} }
            #[ (is_header_valid(HDR($hdrname), pd) ? pd->fields.FLD($hdr,$fld) : ($unspec))
        elif e.expr.node_type == 'MethodCallExpression':
            # note: this is an apply_result_t, it cannot be invalid
            #[ ${format_expr(e.expr)}.${e.member}
        elif e.expr.node_type == 'ArrayIndex':
            hdrname, fldname = get_hdrfld_name(e)
            #[ FLD($hdrname,$fldname)
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


def gen_format_method_parameter(par, listexpr_to_buf):
    if 'expression' in par and (listexpr := par.expression).node_type in ('ListExpression'):
        return listexpr_to_buf[listexpr]

    if 'expression' in par and 'fld_ref' in par.expression:
        expr = par.expression.expr
        hdrname = expr.member
        fldname = expr.fld_ref.name
        #[ handle(header_desc_ins(pd, HDR($hdrname)), FLD($hdrname, $fldname))
    else:
        fmt = format_expr(par)
        if fmt == '':
            return None
        ref = "&" if is_ref(par.expression) else ""
        #[ $ref$fmt

# TODO not needed anymore, remove
def gen_extern_decl(mname, m):
    mret_type = m.urtype.returnType

    with SugarStyle("inline_comment"):
        pars = m.action_ref.urtype.parameters.parameters
        type_pars = m.type.typeParameters.parameters

        def resolve_type_par(node):
            if node.urtype.node_type == 'Type_Name':
                return type_pars.get(node.type.path.name)
            return node

        ptypes = [format_type(resolve_type_par(par).urtype) for par in pars]

    #[ extern ${format_type(mret_type)} ${mname}(${gen_list_elems(ptypes, "SHORT_STDPARAMS")});


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
            #pre[     EXTRACT_BYTEBUF_PACKET(pd, HDR($hdrname), FLD($hdrname,$fldname), $vardata + $varoffset);
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
    #pre[        .buffer_size = $varsize,
    #pre[        .buffer = $vardata,
    #pre}     };

    return buf


def gen_format_call_extern(args, mname, m, funname_override=None):
    if funname_override is not None:
        mname = funname_override

    fmt_args = []
    listexpr_to_buf = {}

    for listexpr in args.map('expression').filter('node_type', 'ListExpression'):
        buf = gen_prepare_listexpr_arg(listexpr)
        listexpr_to_buf[listexpr] = buf

    with SugarStyle("inline_comment"):
        fmt_args += [fmt_arg for arg in args if not arg.is_vec() for fmt_arg in [gen_format_method_parameter(arg, listexpr_to_buf)] if fmt_arg is not None]

    #[     $mname(${gen_list_elems(fmt_args, "SHORT_STDPARAMS_IN")})

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
            #= gen_format_call_extern(args, mname, m, funname_override)
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

        total_width = '+'.join((f'fld_infos[FLD({co.expr.member},{co.fld_ref.name})].bit_width' for co in cos))

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
                width = '+'.join((f'fld_infos[FLD({hdrname},{fld.name})].bit_width' for _, (_, fld) in neighbour_flds))
                #pre[ memcpy(buffer${e.id} + (${offset}+7)/8, get_fld_pointer(pd, FLD($hdrname, ${fld0.name})), ((${width}) +7)/8);

                idxflds = idxflds[len(neighbour_flds):]
                offset += f'+{width}'

    #[ (uint8_buffer_t) { .buffer = buffer${e.id}, .buffer_size = buffer${e.id}_size } /* ListExpression */

def gen_fmt_StructInitializerExpression(e, format_as_value=True, expand_parameters=False, needs_variable=False, funname_override=None):
    #{ (${gen_format_type(e.type)}) {
    for component in e.components:
        ce = component.expression
        tref = ce.expr("ref.urtype")
        if tref and tref.is_metadata:
            #[ .${component.name} = (GET_INT32_AUTO_PACKET(pd, HDR(all_metadatas), FLD(${tref.name},${ce.member}))),
        else:
            if ce.type.size <= 32:
                #[ .${component.name} = ${gen_format_expr(ce)},
            else:
                #[ /* ${component.name}/${ce.type.size}b will be initialised afterwards */
    #} }

def gen_fmt_StructExpression(e, format_as_value=True, expand_parameters=False, needs_variable=False, funname_override=None):
    varname = gen_var_name(e)
    #pre[ ${e.type.name} $varname;
    for component in e.components:
        ce = component.expression
        if ce.node_type == 'Constant':
            #pre[ $varname.${component.name} = ${format_expr(ce)};
        else:
            tref = ce.expr("ref.urtype")
            if tref and tref.is_metadata:
                #pre[ $varname.${component.name} = (GET_INT32_AUTO_PACKET(pd, HDR(all_metadatas), FLD(${tref.name},${ce.member})));
            else:
                if ce.type.size <= 32:
                    #pre[ $varname.${component.name} = ${gen_format_expr(ce)};
                else:
                    bitsize = (ce.type.size+7)//8
                    hdrname, fldname = get_hdrfld_name(ce)
                    #pre[ EXTRACT_BYTEBUF_PACKET(pd, HDR($hdrname), FLD($hdrname,$fldname), &($varname.${component.name}));
    #[ &$varname


def gen_fmt_Constant(e, format_as_value=True, expand_parameters=False, needs_variable=False, funname_override=None):
    if e.type.node_type == 'Type_Bits':
        if e.type.size > 32 or needs_variable:
            name, hex_content = make_const(e)
            const_var = generate_var_name(f"const{e.type.size}_", name)

            #pre[ uint8_t ${const_var}[] = {$hex_content};
            #[ ${const_var}
        else:
            is_not_too_big = e.type.size <= 4 or e.value < 2**(e.type.size-1)
            value_hint = "" if is_not_too_big else f" /* probably -{2**e.type.size - e.value} */"
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
            slicexpr = f'{src} >> {var_offset}'
            #[ ${masking(e.type, slicexpr)}
        elif dst_size % 8 == 0 and src_size % 8 == 0:
            byte_size = 1 if dst_size <= 8 else 2 if dst_size <= 16 else 4
            endian_conversion = f"net2t4p4s_{byte_size}"

            needs_defererencing = e.e0.needs_defererencing if 'needs_defererencing' in e.e0 else True
            deref = "*" if needs_defererencing else ""
            var_slice = generate_var_name(f'slice_{dst_size}b')

            typ = format_type(e.type)
            #TODO change int to $typ

            #pre[ int ${var_slice} = ${endian_conversion}(($deref( ($typ*)(($src) + ($var_offset/8)))));
            #[ ${var_slice}
        else:
            addError('formatting >> operator', f'Unsupported slice: source or destination is not multibyte size)')
    else:
        addError('formatting >> operator', f'Unsupported slice: result is bigger than 32 bits ({size} bits)')


def gen_format_expr(e, format_as_value=True, expand_parameters=False, needs_variable=False, funname_override=None):
    prefix = 'gen_fmt_'
    complex_cases = {name[len(prefix):]: fun for name, fun in funs_with_cond(lambda name: name.startswith(prefix))}

    if e is None:
        addError("Formatting expression", "An expression was found to be None, this should be impossible")
        return "FORMAT_EXPR(None)"

    nt = e.node_type

    if nt == 'Declaration_Instance':
        dinst_name = e.urtype.name
        prefix = '' if dinst_name == 'Digest' else f'{dinst_name}_'
        postfix = f'_{e.packets_or_bytes}' if dinst_name in ('counter', 'meter', 'direct_meter') else ''
        postfix2 = f'_{e.table.name}' if 'table' in e else ''
        # TODO improve this
        if 'smem_type' in e:
            type_name = e.components[0]['type']
        else:
            type_name = f'{dinst_name}_t'
        deref = '' if dinst_name == 'register' else '&'
        smem_name = f'{prefix}{e.name}{postfix}{postfix2}'
        #[ (${type_name}*)${deref}(global_smem.${smem_name})
    elif nt == 'Member' and 'expr' in e and 'expr' in e.expr and e.expr.expr.urtype.node_type == 'Type_Stack':
        fldname = e.member
        idx = e.expr.member
        stk = e.expr.expr.member

        hdr = generate_var_name('stack')
        if idx == 'last':
            #pre[ header_instance_t $hdr = stk_current(STK($stk), pd);
        else:
            #pre[ header_instance_t $hdr = stk_at_idx(STK($stk), $idx, pd);

        #[ (stk_start_fld($hdr) + stkfld_offset_${stk}_$fldname)
    elif nt == 'Member' and 'fld_ref' in e:
        fldname = e.fld_ref.name
        # note: this special case is here because it uses #pre; it should be in 'gen_fmt_Member'
        if not format_as_value:
            #[ FLD(${e.expr.hdr_ref.name}, ${fldname})
        elif e.type.size > 32 or needs_variable:
            var_name = generate_var_name(f"hdr_{e.expr.hdr_ref.name}_{fldname}")
            byte_size = (e.type.size + 7) // 8

            hdrname = e.expr.member

            #pre[ uint8_t ${var_name}[${byte_size}];
            #pre[ EXTRACT_BYTEBUF_PACKET(pd, HDR($hdrname), FLD($hdrname,$fldname), ${var_name});

            #= var_name
        else:
            hdrname = get_hdr_name(e.expr)
            size = e.expr.fld_ref.urtype.size
            unspec = unspecified_value(size)

            #pre{ if (!is_header_valid(HDR($hdrname), pd)) {
            #pre[     debug("   " T4LIT(!!,warning) " Access to field in invalid header " T4LIT(%s,warning) "." T4LIT(${e.member},field) ", returning \"unspecified\" value " T4LIT($unspec) "\n", hdr_infos[HDR($hdrname)].name);
            #pre} }
            #[ (is_header_valid(HDR(${hdrname}), pd) ? GET_INT32_AUTO_PACKET(pd, HDR($hdrname), FLD($hdrname,$fldname)) : ($unspec))
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
            #pre[ uint${sz}_t $fldvar = GET_INT32_AUTO_PACKET(pd,HDR($hdrname),FLD($hdrname,$fldname));
            #pre[ dbg_bytes(&$fldvar, (${size}+7)/8, "        : "T4LIT(${hdrname},header)"."T4LIT(${fldname},field)"/"T4LIT(${size})" = ");
        else:
            #pre[ dbg_bytes(get_fld_pointer(pd, FLD($hdrname,$fldname)), (${size}+7)/8, "        : "T4LIT(${hdrname},header)"."T4LIT(${fldname},field)"/"T4LIT(${size})" = ");

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
            #pre[     add_digest_field($var, get_fld_pointer(pd, FLD($hdrname,$fldname)), $size);
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
    with SugarStyle("no_comment"):
        return gen_format_declaration(d, varname_override)

# TODO use the varname argument in all cases where a variable declaration is created
def format_type(t, varname = None, resolve_names = True, addon = ""):
    with SugarStyle("inline_comment"):
        use_array = varname is not None
        if 'size' in t.urtype and t.urtype.size <= 32:
            use_array = False
        result = gen_format_type(t, resolve_names, use_array=use_array, addon=addon).strip()

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

    ret = gen_format_statement(stmt)

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

def gen_var_name(item, prefix = None):
    if item.node_type == 'Member':
        e = item._expr
        hdrname = e.type.name if e.node_type == 'ArrayIndex' else e.member if 'member' in e else e.path.name
        #[ Member${item.id}_${hdrname}__${item.member}
    else:
        prefix = prefix or f"value_{item.node_type}"
        #[ ${prefix}_${item.id}
