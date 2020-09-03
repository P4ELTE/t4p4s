# SPDX-License-Identifier: Apache-2.0
# Copyright 2017 Eotvos Lorand University, Budapest, Hungary

from inspect import getmembers, isfunction
import sys

from utils.misc import addWarning, addError
from compiler_common import types, with_base, resolve_reference, is_subsequent, groupby, group_references, fldid, fldid2, pp_type_16, method_parameters_by_type, make_const, SugarStyle, prepend_statement, append_statement, is_control_local_var, generate_var_name, pre_statement_buffer, post_statement_buffer, enclosing_control
from hlir16.hlir_attrs import simple_binary_ops, complex_binary_ops

################################################################################

SHORT_STDPARAMS = "packet_descriptor_t* pd, lookup_table_t** tables"
SHORT_STDPARAMS_IN = "pd, tables"
STDPARAMS = f"{SHORT_STDPARAMS}, parser_state_t* pstate"
STDPARAMS_IN = f"{SHORT_STDPARAMS_IN}, pstate"

################################################################################

def type_to_str(t):
    if 'name' in t.urtype and t.urtype.name in types.env():
        translated_type = types.env()[t.urtype.name]
        return type_to_str(translated_type)
    if t.node_type == 'Type_Bits':
        return f'{t.size}'
    return f'TODO_TYPE_{t.name}'

def gen_format_type(t, resolve_names = True, use_array = False, addon = ""):
    """Returns a type. If the type has a part that has to come after the variable name in a declaration,
    such as [20] in uint8_t varname[20], it should be separated with a space."""
    import inspect; import pprint; pprint.PrettyPrinter(indent=4,width=999,compact=True).pprint([f"codegen.sugar.py@{inspect.getframeinfo(inspect.currentframe()).lineno}", t])
    breakpoint()
    if t.node_type == 'Type_Specialized':
        extern_name = t.baseType.path.name

        # TODO is there a more straightforward way to deal with such externs?
        argtyped_externs = ["Digest"]

        if extern_name in argtyped_externs:
            #[ ${t.arguments[0].urtype.name}
        else:
            env = {typename.name: arg for typename, arg in zip(t.urtype.typeParameters.parameters, t.arguments)}
            with types(env):
                raw_types = [par.urtype for par in t.urtype.typeParameters.parameters]
                # postfix_types = [types.env()[t.urtype.name].urtype if t.urtype in types.env() else t.urtype for t in raw_types]
                postfix_types = [t.urtype for t in raw_types]
                postfix = ''.join((f'_{type_to_str(pt)}' for pt in postfix_types))
            #[ extern_${extern_name}${postfix}
    elif t.node_type == 'Type_Void':
        #[ void
    elif t.node_type == 'Type_Boolean':
        #[ bool
    elif t.node_type == 'Type_Bits':
        sign = 'int' if t.isSigned else 'uint'
        base_size = 8 if t.size <= 8 else 16 if t.size <= 16 else 32 if t.size <= 32 else 8
        if use_array:
            #[ ${sign}${base_size}_t $addon[(${t.size} + 7) / 8]
        else:
            postfix = '*' if t.size > 32 else ''
            #[ ${sign}${base_size}_t$postfix $addon
    elif t.node_type == 'Type_Enum':
        #[ enum ${t.c_name}
    elif t.node_type == 'Type_Var' and t.urtype.name in types.env():
        #[ ${types.env()[t.urtype.name]}
    elif t.node_type == 'Type_Name':
        t2 = t.urtype
        if not resolve_names:
            #[ ${t2.name}
        else:
            #= gen_format_type(t2, resolve_names, use_array, addon)
    elif t.node_type == 'Type_Extern':
        #[ ${t.name}_t
    elif t.node_type == 'Type_Struct':
        base_name = re.sub(r'_t$', '', t.name)
        #[ struct ${base_name}_s
    elif t.node_type == 'Type_Varbits':
        #[ uint8_t [${(t.size+7)//8}] /* preliminary type for varbits */
    elif t.node_type == 'Type_Error':
        #[ uint8_t /* preliminary type for the "error" type */
    else:
        addError('formatting type', f'Type {t.node_type} for node ({t}) is not supported yet!')
        #[ int /* generated in place of unknown type ${t.node_type} */

def gen_format_type_mask(t):
    if t.node_type == 'Type_Bits' and not t.isSigned:
        mask = hex((2 ** t.size) - 1)
        #[ $mask&
    else:
        addError('formatting a type mask', 'Currently only bit<w> is supported!')

def format_method_parameter(par):
    if 'field_ref' in par:
        return f'handle(header_desc_ins(pd, {par.expr.header_ref.id}), {par.expression.field_ref.id})'
    else:
        return format_expr(par)

def gen_format_method_parameters(args, method_params):
    return ', '.join((format_method_parameter(par) for par, tpar in method_parameters_by_type(args, method_params)))

def gen_format_declaration(d, varname_override):
    var_name = d.name if varname_override is None else varname_override

    if d.node_type == 'Declaration_Variable':
        if d.type('type_ref.node_type', 'not header') == 'Type_Header':
            # Data for variable width headers is stored in parser_state_t
            pass
        elif d.type.node_type == 'Type_Boolean':
            #[ bool ${var_name} = false;
        else:
            t = gen_format_type(d.type, False)
            #[ $t ${var_name};
    elif d.node_type == 'Declaration_Instance':
        t = f'{gen_format_type(d.type, False)}_t'
        #[ extern void ${t}_init(${t}*);
        #[ $t ${var_name};
        #[ ${t}_init(&${var_name});
    elif d.node_type in ['P4Table', 'P4Action']:
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

def member_to_hdr_fld(member_expr):
    hdrname = "all_metadatas" if member_expr.expr.header_ref('is_metadata', False) else member_expr.expr.type.name
    return f'HDR({hdrname})', f'FLD({hdrname},{member_expr.member})'

def member_to_fld_name(member_expr):
    return member_to_hdr_fld(member_expr)[1]


def gen_extern_format_parameter(expr, par, packets_or_bytes_override = None):
    # TODO
    # if packets_or_bytes_override:
    if par.direction == "in" or expr.node_type != "Member":
        prefix = "&" if par.direction != "in" and par.type.node_type != 'Type_Bits' else ""
        #[ $prefix(${format_expr(expr, format_as_value=True, expand_parameters=True)})
    else:
        expr_width = expr.type.size

        hdrname = "all_metadatas" if expr.expr.ref.urtype.is_metadata else expr.expr.member if 'member' in expr.expr else expr.member
        fldname = member_to_fld_name(expr)

        if expr_width<=32:
            expr_unit = bit_bounding_unit(expr.type)
            #pre[ uint${expr_unit}_t value_${expr.id};
            if par.direction=="inout":
                #pre[ value_${expr.id} = ${format_expr(expr)};
            #aft[ set_field((fldT[]){{pd, HDR($hdrname), ${fldname} }}, 0, value_${expr.id}, ${expr_width});
            #[ &value_${expr.id}
        else:
            #pre[ uint8_t value_${expr.id}[${(int)((expr_width+7)//8)}];
            if par.direction=="inout":
                #pre[ EXTRACT_BYTEBUF_PACKET(pd, HDR(${hdrname}), ${fldname}, value_${expr.id});
            #aft[ MODIFY_BYTEBUF_BYTEBUF_PACKET(pd, HDR(${hdrname}), ${fldname}, value_${expr.id}, ${expr_width});
            #[ value_${expr.id}


def gen_format_statement_fieldref_wide(dst, src, dst_width, dst_is_vw, dst_bytewidth, dst_name, dst_hdr_name, dst_fld_name):
    if src.node_type == 'Member':
        src_pointer = f'tmp_fldref_{src.id}'
        #[ uint8_t $src_pointer[$dst_bytewidth];

        if 'field_ref' in src:
            hdrinst = 'all_metadatas' if src.expr.type.is_metadata else src.expr.member
            #[ EXTRACT_BYTEBUF_PACKET(pd, HDR(${hdrinst}), ${member_to_fld_name(src)}, ${src_pointer})
            if dst_is_vw:
                src_vw_bitwidth = f'pd->headers[HDR({src.expr.member})].var_width_field_bitwidth'
                dst_bytewidth = f'({src_vw_bitwidth}/8)'
        else:
            srcname = src.expr.ref.name
            #[ EXTRACT_BYTEBUF_BUFFER(pstate->${srcname}, pstate->${srcname}_var, ${member_to_fld_name(src)}, $src_pointer)
            if dst_is_vw:
                src_vw_bitwidth = f'pstate->{src.expr.ref.name}_var'
                dst_bytewidth = f'({src_vw_bitwidth}/8)'
    elif src.node_type == 'PathExpression':
        refbase = "local_vars->" if is_control_local_var(src.ref.name) else 'parameters.'
        src_pointer = f'{refbase}{src.ref.name}'
    elif src.node_type == 'Constant':
        src_pointer = f'tmp_fldref_{src.id}'
        #[ uint8_t $src_pointer[$dst_bytewidth] = ${int_to_big_endian_byte_array_with_length(src.value, dst_bytewidth, src.base)};
    elif src.node_type == 'Mux':
        src_pointer = f'tmp_fldref_{src.id}'
        #[ uint8_t $src_pointer[$dst_bytewidth] = ((${format_expr(src.e0.left)}) == (${format_expr(src.e0.right)})) ? (${format_expr(src.e1)}) : (${format_expr(src.e2)});
    else:
        src_pointer = 'NOT_SUPPORTED'
        addError('formatting statement', f'Assignment to unsupported field in: {format_expr(dst)} = {src}')

    dst_fixed_size = dst.expr.header_ref.urtype.size - dst.field_ref.size

    if dst_is_vw:
        #[ pd->headers[$dst_hdr_name].var_width_field_bitwidth = get_var_width_bitwidth(pstate);
        #[ pd->headers[$dst_hdr_name].length = ($dst_fixed_size + pd->headers[$dst_hdr_name].var_width_field_bitwidth)/8;

    #[ MODIFY_BYTEBUF_BYTEBUF_PACKET(pd, $dst_hdr_name, $dst_fld_name, $src_pointer, $dst_bytewidth)

    #[ dbg_bytes($src_pointer, $dst_bytewidth,
    #[      "    " T4LIT(=,field) " Modifying field " T4LIT(%s,header) "." T4LIT(%s,field) "/" T4LIT(%db) " (" T4LIT(%d) "B) = ",
    #[      header_instance_names[$dst_hdr_name],
    #[      field_names[$dst_fld_name], // $dst_fld_name
    #[      $dst_bytewidth*8,
    #[      $dst_bytewidth
    #[      );

def is_primitive(typenode):
    """Returns true if the argument node is compiled to a non-reference C type."""
    # TODO determine better if the source is a reference or not
    return typenode.node_type in ["Type_Boolean"] or (typenode.node_type == 'Type_Bits' and typenode.size <= 32)


def gen_format_statement_fieldref_short(dst, src, dst_width, dst_is_vw, dst_bytewidth, dst_name, dst_hdr_name, dst_fld_name):
    src_buffer = 'value32'
    if src.node_type == 'Member':
        #[ $src_buffer = ${format_expr(src)};
    elif src.node_type == 'PathExpression':
        indirection = "&" if is_primitive(src.type) else ""
        refbase = "local_vars->" if is_control_local_var(src.ref.name) else 'parameters.'
        #[ memcpy(&$src_buffer, $indirection($refbase${src.ref.name}), $dst_bytewidth);
    else:
        #[ $src_buffer = ${format_expr(src)};


    #[ // MODIFY_INT32_INT32_AUTO_PACKET(pd, $dst_hdr_name, $dst_fld_name, $src_buffer)
    #[ set_field((fldT[]){{pd, $dst_hdr_name, $dst_fld_name}}, 0, $src_buffer, $dst_width);


def gen_format_statement_fieldref(dst, src):
    #TODO: handle preparsed fields, width assignment for vw fields and assignment to buffer instead header fields
    dst_width = dst.type.size
    dst_is_vw = dst.type.node_type == 'Type_Varbits'
    dst_bytewidth = (dst_width+7)//8

    assert(dst_width == src.type.size)
    assert(dst_is_vw == (src.type.node_type == 'Type_Varbits'))

    dst_name = dst.expr.member if dst.expr.node_type == 'Member' else dst.expr.path.name if dst.expr('header_ref', lambda h: h.urtype.is_metadata) else dst.expr._header_ref._path.name
    dst_hdr_name, dst_fld_name = member_to_hdr_fld(dst)

    if dst_width <= 32:
        #= gen_format_statement_fieldref_short(dst, src, dst_width, dst_is_vw, dst_bytewidth, dst_name, dst_hdr_name, dst_fld_name)
    else:
        #= gen_format_statement_fieldref_wide(dst, src, dst_width, dst_is_vw, dst_bytewidth, dst_name, dst_hdr_name, dst_fld_name)


def is_atomic_block(blckstmt):
    try:
        return any((annot.name == "atomic" for annot in blckstmt.annotations.annotations.vec))
    except:
        return False


def gen_do_assignment(dst, src):
    if dst.type.node_type == 'Type_Header':
        #[ // TODO make it work properly for non-byte-aligned headers
        if 'member' in dst:
            if 'member' in src:
                #[ memcpy(pd->headers[header_instance_${dst.member}].pointer, pd->headers[header_instance_${src.member}].pointer, hdr_infos[header_instance_${src.member}].byte_width);
                #[ dbg_bytes(pd->headers[header_instance_${dst.member}].pointer, hdr_infos[header_instance_${src.member}].byte_width, "Copied %02d bytes from header_instance_${src.member} to header_instance_${dst.member}: ", hdr_infos[header_instance_${src.member}].byte_width);
            else:
                addError("Compiling assignment", f"Assigning to header instance {dst.member} (type {dst.type.name}) from {src.ref.name} ({src.type.name}) is not supported")
        else:
            addError("Compiling assignment", f"Assigning {dst.ref.name} (type {dst.type.name}) from {src.ref.name} ({src.type.name}) is not supported")
    elif dst.type.node_type == 'Type_Bits':
        # TODO refine the condition to find out whether to use an assignment or memcpy
        requires_memcpy = src.type.size > 32
        is_assignable = src.type.size in [8, 32]

        if src.type.node_type == 'Type_Bits' and not requires_memcpy:
            if is_assignable:
                # TODO how to decide whether src is a pointer, and therefore needs dereferencing?
                needs_defererencing = src.node_type not in ["Constant"]
                # needs_defererencing = src('field_ref', lambda fr: fr.name == 'meta')
                dereference = "*" if needs_defererencing else ""

                if dst("expr.ref.type.type_ref.is_metadata"):
                    #[ set_field((fldT[]){{pd, HDR(all_metadatas), FLD(${dst.expr.ref.type.type_ref.name},${dst.member})}}, 0, ($dereference(${format_expr(src, expand_parameters=True)})), ${dst.type._type_ref.size});
                    #[ debug("       : " T4LIT(all_metadatas,header) "." T4LIT(${dst.expr.ref.type.type_ref.name}_${dst.member},field) "/" T4LIT(%d) " = " T4LIT(%d,bytes) " (" T4LIT(%${(src.type.size+7)//8}x,bytes) ")\n", ${dst.type._type_ref.size}, $dereference(${format_expr(src, expand_parameters=True)}), $dereference(${format_expr(src, expand_parameters=True)}));
                elif dst("header_ref.type.type_ref.is_metadata"):
                    #[ set_field((fldT[]){{pd, HDR(all_metadatas), FLD(${dst.header_ref.type.type_ref.name},${dst.field_name})}}, 0, ($dereference(${format_expr(src, expand_parameters=True)})), ${dst.type._type_ref.size});
                    #[ debug("       : " T4LIT(all_metadatas,header) "." T4LIT(${dst.header_ref.type.type_ref.name}_${dst.field_name},field) "/" T4LIT(%d) " = " T4LIT(%d,bytes) " (" T4LIT(%${(src.type.size+7)//8}x,bytes) ")\n", ${dst.type._type_ref.size}, $dereference(${format_expr(src, expand_parameters=True)}), $dereference(${format_expr(src, expand_parameters=True)}));
                else:
                    #[ ${format_expr(dst)} = (${format_type(dst.type)})($dereference(${format_expr(src, expand_parameters=True)}));
                    if dst.node_type == 'Member':
                        if dst.type('is_metadata', lambda ismeta: ismeta):
                            # Note: the metadata header and field name is joined by underscores, separating them as best as possible
                            nameparts = dst.member.split("_")
                            hdr = "_".join(nameparts[1:-1])
                            fld = nameparts[-1]
                        else:
                            hdr = dst.expr._expr.path.name
                            fld = dst.member
                        #[ debug("       : " T4LIT($hdr,header) "." T4LIT($fld,field) " = " T4LIT(%d,bytes) " (" T4LIT(%${(src.type.size+7)//8}x,bytes) ")\n", ${format_expr(dst)}, ${format_expr(dst)});
                    else:
                        #[ debug("       : " T4LIT(${format_expr(dst)},header) " = " T4LIT(%d,bytes) " (" T4LIT(%${(src.type.size+7)//8}x,bytes) ")\n", ${format_expr(dst)}, ${format_expr(dst)});
            else:
                tmpvar = generate_var_name()
                #[ ${format_type(dst.type)} $tmpvar = ${format_expr(src, expand_parameters=True)};
                #[ ${format_expr(dst)} = $tmpvar;

                # TODO this part should not require memcpy
                # [ ${format_type(dst.type)} $tmpvar = ${format_expr(src, expand_parameters=True)};
                # [ memcpy(&(${format_expr(dst)}), &$tmpvar, sizeof(${format_type(dst.type)}));
                # TODO debug printout
        else:
            #[ memcpy(&(${format_expr(dst)}), &(${format_expr(src, expand_parameters=True)}), ${dst.type.size});
            #[ dbg_bytes(&(${format_expr(src, expand_parameters=True)}), ${dst.type.size}, "Copied " T4LIT(%02d) " bytes from ${format_expr(src, expand_parameters=True)} to ${format_expr(dst)}: ", ${dst.type.size});
    else:
        #[ ${format_expr(dst)} = ${format_expr(src, expand_parameters=True)};


def gen_format_statement(stmt):
    if stmt.node_type == 'AssignmentStatement':
        dst = stmt.left
        src = stmt.right
        if 'field_ref' in dst:
            #= gen_format_statement_fieldref(dst, src)
        else:
            #= gen_do_assignment(dst, src)
    elif stmt.node_type == 'BlockStatement':
        is_atomic = is_atomic_block(stmt)
        if is_atomic:
            #[ LOCK(&${compiler_common.enclosing_control.type.name}_lock)
        for c in stmt.components:
            #= gen_format_statement(c)
        if is_atomic:
            #[ UNLOCK(&${compiler_common.enclosing_control.type.name}_lock)
    elif stmt.node_type == 'IfStatement':
        t = format_statement(stmt.ifTrue) if 'ifTrue' in stmt else ';'
        f = format_statement(stmt.ifFalse) if 'ifFalse' in stmt else ';'
        cond = format_expr(stmt.condition)

        # TODO this happens when .hit() is called; make a proper solution
        if cond.strip() == '':
            cond = "true"

        #{ if( $cond ) {
        #[     $t
        #}
        #{ } else {
        #[     $f
        #} }
    elif stmt.node_type == 'MethodCallStatement':
        m = stmt.methodCall.method

        if m.node_type == 'Method' and m.name == 'digest':
            return gen_format_methodcall_digest(stmt, m)
        elif 'member' in m:
            return gen_fmt_methodcall(stmt, m)
        else:
            #= gen_methodcall(stmt)
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

def gen_format_methodcall_digest(stmt, m):
    digest_name = stmt.methodCall.typeArguments[0].name
    port, fields = stmt.methodCall.arguments

    #[ struct type_field_list fields;
    #[ fields.fields_quantity = ${len(fields)};
    #[ fields.field_offsets = malloc(sizeof(uint8_t*)*fields.fields_quantity);
    #[ fields.field_widths = malloc(sizeof(uint8_t*)*fields.fields_quantity);

    for idx, f in enumerate(fields.components):
        if f.expr.type.is_metadata:
            #[ fields.field_offsets[$idx] = (uint8_t*) field_desc(pd, FLD(${f.expr.name},${f.member})).byte_addr;
            #[ fields.field_widths[$idx]  =            field_desc(pd, FLD(${f.expr.name},${f.member})).bitwidth;
        else:
            #[ fields.field_offsets[$idx] = (uint8_t*) field_desc(pd, FLD(${f.expr.member},${f.expression.field_ref.name})).byte_addr;
            #[ fields.field_widths[$idx]  =            field_desc(pd, FLD(${f.expr.member},${f.expression.field_ref.name})).bitwidth;
    #[ generate_digest(bg,"${digest_name}",0,&fields);
    #[ sleep_millis(DIGEST_SLEEP_MILLIS);

def is_emit(stmt, m):
    return m.expr._ref('type')._type_ref('name', lambda n: n == 'packet_out')

def gen_fmt_methodcall(stmt, m):
    if is_emit(stmt, m):
        arg = stmt.methodCall.arguments[0]
        hdr = arg.expression.member
        hdr_type = arg.expression.type

        hdr_name = arg.expression.header_ref.name if 'header_ref' in arg.expression else arg.expression.member

        #[ pd->header_reorder[pd->emit_hdrinst_count] = hdr_infos[HDR($hdr)].idx;
        #[ ++pd->emit_hdrinst_count;
    elif (m.expr.node_type, m.expr('ref').node_type, m.member) == ('PathExpression', 'P4Table', 'apply'):
        #[ ${gen_method_apply(stmt.methodCall)};
    elif m.expr.get_attr('member') is None:
        return gen_fmt_methodcall_extern(stmt, m)
    else:
        hdr_name = m.expr.member

        if m.member == 'isValid':
            #[ controlLocal_tmp_0 = (pd->headers[HDR($hdr_name)].pointer != NULL);
        elif m.member == 'setValid':
            #{ if (likely(pd->headers[HDR($hdr_name)].pointer == NULL)) {
            #[    pd->headers[HDR($hdr_name)].pointer = (pd->header_tmp_storage + hdr_infos[HDR($hdr_name)].byte_offset);
            #[    pd->is_emit_reordering = true;
            #[    debug("   :: Header instance $$[header]{hdr_name}/" T4LIT(%dB) " set as $$[success]{}{valid}\n", pd->headers[HDR($hdr_name)].length);
            #[ } else {
            #[    debug("   " T4LIT(!!,warning) " Trying to set header instance $$[header]{hdr_name} to $$[success]{}{valid}, but it is already $$[success]{}{valid}\n");
            #} }
        elif m.member == 'setInvalid':
            #{ if (likely(pd->headers[HDR($hdr_name)].pointer != NULL)) {
            #[    pd->headers[HDR($hdr_name)].pointer = NULL;
            #[    pd->is_emit_reordering = true;
            #[    debug("   :: Header instance $$[header]{hdr_name}/" T4LIT(%dB) " set as $$[status]{}{invalid}\n", pd->headers[HDR($hdr_name)].length);
            #[ } else {
            #[    debug("   " T4LIT(!!,warning) " Trying to set header instance $$[header]{hdr_name} to $$[success]{}{valid}, but it is already $$[success]{}{valid}\n");
            #} }
        else:
            #= gen_methodcall(stmt)

def gen_fmt_methodcall_extern(stmt, m):
    breakpoint()
    smem_type = m.expr.urtype.name

    # if the extern is about both packets and bytes, it takes two separate calls
    is_possibly_multiple = smem_type in ["counter", "meter", "direct_counter", "direct_meter"]
    if is_possibly_multiple:
        if m.expr.ref.packets_or_bytes == "packets_and_bytes":
            #= gen_format_extern_single(stmt, m, smem_type, is_possibly_multiple, "packets")
            #= gen_format_extern_single(stmt, m, smem_type, is_possibly_multiple, "bytes")
        else:
            #= gen_format_extern_single(stmt, m, smem_type, is_possibly_multiple, m.expr.ref.packets_or_bytes)
    else:
        #= gen_format_extern_single(stmt, m, smem_type, is_possibly_multiple)


def gen_fill_extern_parameter_digest(expr, mprefix, mname, m, stmt, paramtype):
    tmpvar = f'{mprefix}{m.expr.path.name}'

    for fld, component in zip(expr.expression.type.fields, stmt.methodCall.arguments[0].expression.components):
        ce = component.expression

        if ce.node_type == 'Member':
            cee = ce.expr
            if 'field_ref' in ce:
                hdrname, fldname = (cee.header_ref.name, ce.field_ref.name)
            elif 'header_ref' in cee and 'member' in ce:
                hdrname, fldname = (cee.header_ref.name, ce.member)
            elif 'ref' in cee:
                hdrname, fldname = (cee.ref.name, ce.member)
            else:
                hdrname, fldname = (cee.member, ce.member)

            #[ EXTRACT_BYTEBUF_PACKET(pd, HDR($hdrname), FLD(${hdrname},${fldname}), &($tmpvar.${fld.name}));
            #[ dbg_bytes(&($tmpvar.${fld.name}), (${fld.type.size}+7)/8, "       : " T4LIT(${component.name},field) " = ");
        elif ce.node_type == 'Constant':
            name, hex_content = make_const(ce)
            const_var = generate_var_name(f"const{fld.size}", name)

            #pre[ uint8_t ${const_var}[] = {$hex_content};
            #[ memcpy(&($tmpvar.${component.name}), ${const_var}, (${fld.size}+7)/8);
            #[ dbg_print(&($tmpvar.${component.name}), ${fld.size}, "       : " T4LIT(${component.name},field));

def gen_fill_extern_parameter_smem(is_possibly_multiple, expr, mprefix, mname, m, stmt, paramtype):
    # if is_possibly_multiple and 'fields' in expr.expression.type:
    if is_possibly_multiple and 'fields' in expr.expression.type:
        tmpvar = generate_var_name()
        #[ $paramtype $tmpvar = ${gen_extern_format_parameter(expr, par, packets_or_bytes_override)};

        for fld, component in zip(expr.expression.type.fields, stmt.methodCall.arguments[0].expression.components):
            ce = component.expression

            if fld.type.size <= 32:
                #[ dbg_bytes(&($tmpvar.${fld.name}), (${fld.type.size}+7)/8, "       : " T4LIT(${format_expr(ce.expr)},header) "." T4LIT(${ce.member},field) " = ");
                continue

            hdr = ce.expr.header_ref.name

            #[ EXTRACT_BYTEBUF_PACKET(pd, HDR($hdr), FLD(${hdr},${ce.field_ref.name}), &($tmpvar.${fld.name}));
            #[ dbg_bytes(&($tmpvar.${fld.name}), (${fld.type.size}+7)/8, "       : " T4LIT(${hdr},header) "." T4LIT(${ce.field_ref.name},field) " = ");

        #[ memcpy(&($mname), &$tmpvar, sizeof($paramtype));

def gen_fill_extern_parameter(smem_type, is_possibly_multiple, expr, mprefix, mname, m, stmt, paramtype):
    if smem_type == 'Digest':
        #= gen_fill_extern_parameter_digest(expr, mprefix, mname, m, stmt, paramtype)
    else:
        #= gen_fill_extern_parameter_smem(is_possibly_multiple, expr, mprefix, mname, m, stmt, paramtype)


def gen_format_extern_single(stmt, m, smem_type, is_possibly_multiple, packets_or_bytes = None):
    mexpr_type = m.expr.type

    if m.expr.type.node_type == "Type_SpecializedCanonical":
        mexpr_type = mexpr_type.substituted

    parameters = stmt.methodCall.method.type.parameters.parameters

    method_args = list(zip(stmt.methodCall.arguments, parameters))

    mprefix = "global_smem."
    mname = mprefix + m.expr.path.name
    mparname = mname

    if smem_type in ["counter", "meter"]:
        mname = f"{mprefix}{smem_type}_{m.expr.path.name}_{packets_or_bytes}"
        mparname = mname
    if smem_type in ["direct_counter", "direct_meter"]:
        mname = f"{mprefix}{smem_type}_{m.expr.path.name}_{packets_or_bytes}_{'TODO_table'}"
        mparname = mname

    packets_or_bytes_override = None
    if method_args != []:
        (expr, par) = method_args[0]
        breakpoint()
        if m.expr.ref._packets_or_bytes == "packets_and_bytes":
            packets_or_bytes_override = packets_or_bytes

    if m.expr.type.node_type == "Type_Extern":
        # TODO support parameters of type Type_List (e.g. in InternetChecksum in the example psa-l3fwd-with-chksm)
        stypename = stmt.methodCall.method.expr.type.name
        paramtypes = [f'{stypename}_t*' if idx == 0 else gen_format_type(arg[0].expression.type) for idx, arg in enumerate(method_args)]
    elif m.expr.type.node_type == "Type_SpecializedCanonical":
        method_args = method_args[1:]

        if 'name' in expr.expression:
            paramtype = f"struct {expr.expression.name}"
        elif expr.expression.type.node_type == 'Type_Bits':
            paramtype = format_type(expr.expression.type)
        elif expr.expression.node_type == 'Constant':
            paramtype = format_type(expr.expression.type)
        elif m.expr.ref.type.baseType.type_ref.node_type == 'Type_Extern':
            paramtype = m.expr.type.arguments[0].name
        else:
            paramtype = "int/*temporarily inserted for unknown type*/"
            addWarning('generating method call statement', f'Unexpected type {m.expr.type} in {stmt.methodCall}')

        #= gen_fill_extern_parameter(smem_type, is_possibly_multiple, expr, mprefix, mname, m, stmt, paramtype)
        paramtypes = [paramtype]


    def resolve_type(t, type_params):
        return type_params[t.name] if t.node_type == 'Type_Var' else t

    extern_param_indexes = {
        ('meter',        'execute_meter'): ([1]),
        ('register',     'read'):          ([0]),
        ('register',     'write'):         ([1]),
        ('Digest',       'pack'):          ([0]),
    }

    base_type = m.expr.ref.type
    if 'baseType' in base_type:
        base_type = base_type.baseType

    extern_type = base_type.type_ref.name

    type_param_names = [t.name for t in stmt.methodCall.method.type.typeParameters.parameters]
    type_params = dict(zip(type_param_names, stmt.methodCall.typeArguments))
    param_indexes = extern_param_indexes[(extern_type, m.member)] if (extern_type, m.member) in extern_param_indexes else []
    types = [m.type.parameters.parameters[par].type for par in param_indexes]

    # the indexes of the parameters which originate from a type parameter
    # TODO generalize and move to hlir_attrs
    default_extern_opts = (True, [], None, None)
    # TODO add a way to issue compile time/runtime warnings (addWarning), e.g. if there is a buffer overflow

    externs = {
        ('bytes',   'meter',        'execute_meter'): ( True, [],        ["{0}", "{2}", "{3}"],            ["{0}[(uint32_t)({1}) - 1]", "*((uint32_t*)({2}))"]),
        ('packets', 'counter',      'count'):         ( True, [],        ["{0}", "int", "uint32_t"],       ["{0}[(uint32_t)({1}) - 1]", "{1}", "1"]),
        ('bytes',   'counter',      'count'):         ( True, [],        ["{0}", "int", "uint32_t"],       ["REPLACED_BELOW"]),
        (None,      'register',     'read'):          ( True, ["&({})"], ["register_{0}*", "{1}*", "{2}"], None),
        (None,      'register',     'write'):         ( True, ["{}"],    ["register_{0}*", "int", "{2}"],  ["{0}[(uint32_t)({1}) - 1]", "(int)({1})", "{2}"]),
        (None,      'Digest',       'pack'):          (False, [],        ["{1}*"],                         None),
    }

    extern_params = (packets_or_bytes, extern_type, m.member)
    type_args_in_fun_name, expr_args, type_par_reformat, arg_reformat = externs[extern_params] if extern_params in externs else default_extern_opts

    if packets_or_bytes == 'bytes':
        varname = generate_var_name("packet_size_bytes")
        #pre[ uint32_t $varname = pd->parsed_length;
        arg_reformat = ["{0}[(uint32_t)({1}) - 1]", "{1}", varname]


    if expr_args != []:
        ee = expr.expression
        # TODO if 'meta', use the appropriate field of all_metadatas
        local_name = str(ee.value) if ee.node_type == "Constant" else mprefix + ee.member if ee._expr.path.name == 'meta' else mprefix + ee.path.name
        expr_args = [earg.format(local_name) for earg in expr_args]

    type_params2 = paramtypes + [format_type(resolve_type(par.type, type_params)) for par in parameters]
    if type_par_reformat is not None:
        type_params2 = [fmt.format(*type_params2) for fmt in type_par_reformat]
    type_params_str = ", ".join(type_params2)

    type_args = "".join([f"_{format_type(resolve_type(t, type_params))}" for t in types])

    with SugarStyle("inline_comment"):
        param_args = [gen_extern_format_parameter(arg.expression, par) for (arg, par) in method_args]

    all_args = [mparname] + expr_args + param_args

    if arg_reformat is not None:
        all_args = [fmt.format(*all_args) for fmt in arg_reformat]
    all_args = ["&({})".format(all_args[0])] + all_args[1:]
    all_args = ", ".join(all_args)

    funname_postfix = type_args if type_args_in_fun_name else ""

    #pre[ extern void extern_${mexpr_type.name}_${m.member}${funname_postfix}(${type_params_str});
    #[ extern_${mexpr_type.name}_${m.member}${funname_postfix}(${all_args});



def gen_methodcall(stmt):
    mcall = format_expr(stmt.methodCall)

    if mcall:
        #[ $mcall;
    else:
        addWarning('generating method call statement', f'Invalid method call {stmt.methodCall}')
        #[ /* unhandled method call ${stmt.methodCall} */


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

def gen_method_isValid(e):
    if 'header_ref' in e.method.expr:
        #[ (pd->headers[${e.method.expr.header_ref.id}].pointer != NULL)
    else:
        #[ (pd->headers[${format_expr(e.method.expr)}].pointer != NULL)

def gen_method_setInvalid(e):
    if 'header_ref' in e.method.expr:
        #[ pd->headers[${e.method.expr.header_ref.id}].pointer = NULL
    else:
        #[ pd->headers[${format_expr(e.method.expr)}].pointer = NULL

def gen_method_apply(e):
    #[ ${e.method.expr.path.name}_apply(STDPARAMS_IN)

def gen_method_setValid(e):
    h = e.method.expr.header_ref

    # TODO fix: f must always have an is_vw attribute
    def is_vw(f):
        if f.get_attr('is_vw') is None:
            return False
        return f.is_vw

    # TODO is this the max size?
    length = (sum([f.size if not is_vw(f) else 0 for f in h.type.type_ref.fields])+7)//8

    #[ pd->headers[${h.id}] = (header_descriptor_t) {
    #[     .type = ${h.id},
    #[     .length = $length,
    #[     .pointer = calloc(${h.type.type_ref.byte_width}, sizeof(uint8_t)),
    #[     /*TODO determine and set this field*/
    #[     .var_width_field_bitwidth = 0,
    #[ };


def gen_fmt_Cast(e, format_as_value=True, expand_parameters=False):
    et = e.expr.type
    edt = e.destType
    if (et.node_type, et.size, edt.node_type) == ('Type_Bits', 1, 'Type_Boolean') and not et.isSigned:
        #Cast from bit<1> to bool
        return f"({format_expr(e.expr)})"
    elif (et.node_type, edt.node_type, edt.size) == ('Type_Boolean', 'Type_Bits', 1) and not edt.isSigned:
        #Cast from bool to bit<1>
        return f'({format_expr(e.expr)} ? 1 : 0)'
    elif et.node_type == 'Type_Bits' and edt.node_type == 'Type_Bits':
        if et.isSigned == edt.isSigned:
            if not et.isSigned:                       #Cast from bit<w> to bit<v>
                if et.size > edt.size:
                    return f'({format_type_mask(e.destType)}{format_expr(e.expr)})'
                else:
                    return format_expr(e.expr)
            else:                                              #Cast from int<w> to int<v>
                return f'(({format_type(e.destType)}) {format_expr(e.expr)})'
        elif et.isSigned and not edt.isSigned: #Cast from int<w> to bit<w>
            return f'({format_type_mask(e.destType)}{format_expr(e.expr)})'
        elif not et.isSigned and edt.isSigned: #Cast from bit<w> to int<w>
            if edt.size in {8,16,32}:
                return f'(({format_type(e.destType)}){format_expr(e.expr)})'
            else:
                addError('formatting an expression', f'Cast from bit<{et.size}> to int<{edt.size}> is not supported! (Only int<8>, int<16> and int<32> are supported.)')
                return ''
    #Cast from int to bit<w> and int<w> are performed by P4C
    addError('formatting an expression', f'Cast from {pp_type_16(et)} to {pp_type_16(edt)} is not supported!')
    return ''

def gen_fmt_ComplexOp(e, op, format_as_value=True, expand_parameters=False):
    temp_expr = f'({format_expr(e.left)}{op}{format_expr(e.right)})'
    if e.type.node_type == 'Type_InfInt':
        return temp_expr
    elif e.type.node_type == 'Type_Bits':
        if not e.type.isSigned:
            return f'({format_type_mask(e.type)}{temp_expr})'
        else:
            if e.type.size in {8,16,32}:
                return f'(({format_type(e.type)}){temp_expr})'
            else:
                addError('formatting an expression', f'Expression of type {e.node_type} is not supported on int<{e.type.size}>. (Only int<8>, int<16> and int<32> are supported.)')
                return ''

def gen_fmt_SelectExpression(e, format_as_value=True, expand_parameters=False):
    #Generate local variables for select values
    for k in e.select.components:
        varname = gen_var_name(k)
        if k.type.node_type == 'Type_Bits' and k.type.size <= 32:
            #pre[ ${format_type(k.type)} $varname = ${format_expr(k)};
        elif k.type.node_type == 'Type_Bits' and k.type.size % 8 == 0:
            #pre[ uint8_t $varname[${k.type.size/8}];
            #pre[ EXTRACT_BYTEBUF_PACKET(pd, ${format_expr(k, False)}, $varname);'
        else:
            addError('formatting select expression', f'Select on type {pp_type_16(k.type)} is not supported!')

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
                #pre[ uint8_t ${gen_var_name(c)}[${size/8}] = ${byte_array};
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
        cases.append('if({0}){{parser_state_{1}(pd, buf, tables, pstate);}}'.format(' && '.join(conds), format_expr(case.state)))
    return '\nelse\n'.join(cases)

def gen_fmt_Member(e, format_as_value=True, expand_parameters=False):
    if 'field_ref' in e:
        if format_as_value == False:
            return fldid(e.expr.header_ref, e.field_ref)

        if e.type.size > 32:
            var_name = generate_var_name(f"hdr_{e.expr.header_ref.id}_{e.field_ref.id}")
            byte_size = (e.type.size + 7) // 8

            #pre[ uint8_t* ${var_name}[${byte_size}];
            #pre[ EXTRACT_BYTEBUF_PACKET(pd, ${e.expr.header_ref.id}, ${e.field_ref.id}, ${var_name});

            return var_name

        hdrinst = 'HDR(all_metadatas)' if e.expr.header_ref.urtype.is_metadata else e.expr.header_ref.id
        return f'(GET_INT32_AUTO_PACKET(pd, {hdrinst}, {e.field_ref.id}))'
    elif 'header_ref' in e:
        # TODO do both individual meta fields and metadata instance fields
        if e.header_ref.name == 'metadata':
            #[ pd->fields.FLD(${e.expr.member},${e.member})
        return e.header_ref.id
    elif e.expr.node_type == 'PathExpression':
        hdr = e.expr.ref.name

        if e.expr.type.node_type == 'Type_Header':
            h = e.expr.type
            return f'(GET_INT32_AUTO_PACKET(pd, HDR({hdr}), FLD({h.name},{e.member})))'
        else:
            import pprint; pprint.PrettyPrinter(indent=4).pprint(["DBG codegen.sugar.py:815:source.python  ", e, format_expr(e.expr)])
            #[ ${format_expr(e.expr)}.${e.member}
    else:
        if e.type.node_type in {'Type_Enum', 'Type_Error'}:
            #[ ${e.type.members.get(e.member).c_name}
        elif e.expr('expr', lambda e2: e2.type.name == 'parsed_packet'):
            #[ pd->fields.FLD(${e.expr.member},${e.member})
        else:
            #[ ${format_expr(e.expr)}.${e.member}

def gen_fmt_MethodCallExpression(e, format_as_value=True, expand_parameters=False):
    import inspect; import pprint; pprint.PrettyPrinter(indent=4,width=999,compact=True).pprint([f"codegen.sugar.py@{inspect.getframeinfo(inspect.currentframe()).lineno}", "mcallexpr"])
    # TODO some of these are formatted as statements, we shall fix this
    special_methods = {
        ('Member', 'setValid'):     gen_method_setValid,
        ('Member', 'isValid'):      gen_method_isValid,
        ('Member', 'setInvalid'):   gen_method_setInvalid,
        ('Member', 'apply'):        gen_method_apply,
    }

    method = special_methods.get((e.method.node_type, e.method.member)) if e.method.get_attr('member') is not None else None

    if method:
        #[ ${method(e)}
    elif e.arguments.is_vec() and e.arguments.vec != []:
        # if e.method.get_attr('ref') is None:
        #     mref = e.method.expr.ref
        # else:
        #     mref = e.method.ref
        mname = e.method.path.name

        if mname == 'digest':
            return gen_format_call_digest(e)
        else:
            breakpoint()
            mtype = e.method.urtype
            return gen_format_call_extern(e, mname, mtype.returnType, mtype.typeParameters)
    else:
        if e.method('expr').type.node_type == 'Type_Extern':
            if e.method.member in {'lookahead', 'advance', 'length'}:
                raise NotImplementedError(f'{e.method.expr.type.name}.{e.method.member} is not supported yet!')

            funname = f"{e.method.expr.type.name}_t_{e.method.member}"
            extern_inst = format_expr(e.method.expr)
            extern_type = format_type(e.method.expr.urtype.methods.get(e.method.member, 'Method').urtype.returnType)
            #pre[ extern ${extern_type} ${funname}(${e.method.expr.urtype.name}_t);
            #[ $funname($extern_inst)
        else:
            funname = format_expr(e.method)
            #pre[ extern void ${funname}(SHORT_STDPARAMS);
            #[ $funname(SHORT_STDPARAMS_IN)

def gen_fmt_ListExpression(e, format_as_value=True, expand_parameters=False):
    if e.id not in generated_exprs:
        def fld_width(hdr, fld):
            return f'field_desc(pd, {fldid(hdr, fld)}).bitwidth' if fld.is_vw else f'{fld.size}'

        offset = '0'
        copies = ""
        # TODO add support for component.node_type == 'Constant'
        components = [('tuple', c[0], c[1]) if type(c) == tuple else convert_component(c) for c in map(resolve_reference, e.components)]
        components = [(c[1], c[2]) for c in components if c is not None if c[0] != 'Constant']
        for h, fs in group_references(components):
            width = '+'.join((fld_width(h, f) for f in fs))
            copies += f'memcpy(buffer{e.id} + ({offset}+7)/8, field_desc(pd, {fldid(h, fs[0])}).byte_addr, ({width} +7)/8);\n'
            offset += f'+{width}'

        #pre[ int buffer${e.id}_size = (${offset} +7)/8;
        #pre[ uint8_t buffer${e.id}[buffer${e.id}_size];
        #pre[ ${copies}

        generated_exprs.add(e.id)
    return f'(struct uint8_buffer_s) {{ .buffer =  buffer{e.id}, .buffer_size = buffer{e.id}_size }}'

def gen_fmt_StructInitializerExpression(e, format_as_value=True, expand_parameters=False):
    #{ (${gen_format_type(e.type)}) {
    for component in e.components:
        tref = component.expression.expr("ref.type.type_ref")
        if tref and tref.is_metadata:
            #[ .${component.name} = (GET_INT32_AUTO_PACKET(pd, HDR(all_metadatas), field_${tref.name}_${component.expression.member})),
        else:
            if component.expression.type.size <= 32:
                #[ .${component.name} = ${gen_format_expr(component.expression)},
            else:
                #[ /* ${component.name}/${component.expression.type.size}b will be initialised afterwards */
    #} }

def gen_fmt_StructExpression(e, format_as_value=True, expand_parameters=False):
    varname = gen_var_name(e)
    #pre[ ${format_type(e.type)} $varname;
    for component in e.components:
        tref = component.expression.expr("ref.type.type_ref")
        if tref and tref.is_metadata:
            #pre[ $varname.${component.name} = (GET_INT32_AUTO_PACKET(pd, HDR(all_metadatas), field_${tref.name}_${component.expression.member}));
        else:
            if component.expression.type.size <= 32:
                #pre[ $varname.${component.name} = ${gen_format_expr(component.expression)};
            else:
                indirection = "&" if is_primitive(component.expression.type) else ""
                refbase = "local_vars->" if is_control_local_var(component.name) else 'parameters.'
                bitsize = (component.expression.type.size+7)//8
                hdrinst = component.expression.expr.header_ref.name
                fldinst = component.expression.member
                #pre[ EXTRACT_BYTEBUF_PACKET(pd, header_instance_$hdrinst, field_${hdrinst}_$fldinst, &($varname.${component.name}));
    #[ $varname


def gen_fmt_Constant(e, format_as_value=True, expand_parameters=False):
    if e.type.node_type == 'Type_Bits':
        if e.type.size > 32:
            name, hex_content = make_const(e)
            const_var = generate_var_name(f"const{e.type.size}", name)

            #pre[ uint8_t ${const_var}[] = {$hex_content};
            #[ ${const_var}
        else:
            value_hint = "" if e.type.size <= 4 or e.value < 2**(e.type.size-1) else f" /* probably -{2**e.type.size - e.value} */"
            # 4294967136 versus (uint32_t)4294967136
            #[ (${format_type(e.type)})${with_base(e.value, e.base)}${value_hint}
    else:
        #[ ${e.value}


def gen_fmt_Operator(e, nt, format_as_value=True, expand_parameters=False):
    if nt == 'Neg':
        if e.type.node_type == 'Type_Bits' and not e.type.isSigned:
            return f'({format_type_mask(e.type)}({2**e.type.size}-{format_expr(e.expr)}))'
        else:
            return f'(-{format_expr(e.expr)})'
    elif nt == 'Cmpl':
        return f'({format_type_mask(e.type)}(~{format_expr(e.expr)}))'
    elif nt == 'LNot':
        return f'(!{format_expr(e.expr)})'
    elif nt in simple_binary_ops:
        if nt == 'Equ' and e.left.type.size > 32:
            return f"0 == memcmp({format_expr(e.left)}, {format_expr(e.right)}, ({e.left.type.size} + 7) / 8)"
        else:
            op = simple_binary_ops[nt]
            return f'(({format_expr(e.left)}){op}({format_expr(e.right)}))'
    elif nt in complex_binary_ops:
        if nt == 'Sub' and e.type.node_type == 'Type_Bits' and not e.type.isSigned:
            #Subtraction on unsigned values is performed by adding the negation of the second operand
            return f'({format_type_mask(e.type)}({format_expr(e.left)}+({2 ** e.type.size}-{format_expr(e.right)})))'
        elif nt == 'Shr' and e.type.node_type == 'Type_Bits' and e.type.isSigned:
            #Right shift on signed values is performed with a shift width check
            return '(({1}>{2}) ? 0 : ({0} >> {1}))'.format(format_expr(e.left), format_expr(e.right), e.type.size)
        else:
            #These formatting rules MUST follow the previous special cases
            return gen_fmt_ComplexOp(e, complex_binary_ops[nt], format_as_value, expand_parameters)


def gen_format_expr(e, format_as_value=True, expand_parameters=False):
    prefix = 'gen_fmt_'
    complex_cases = {name[len(prefix):]: fun for name, fun in getmembers(sys.modules[__name__], isfunction) if name.startswith(prefix)}

    # breakpoint()

    if e is None:
        return "FORMAT_EXPR(None)"

    if e.is_vec():
        print(e)

    nt = e.node_type

    if nt in complex_cases:
        return complex_cases[e.node_type](e, format_as_value, expand_parameters)
    elif nt in simple_binary_ops or nt in complex_binary_ops or nt in ('Neg', 'Cmpl', 'LNot'):
        return gen_fmt_Operator(e, nt, format_as_value, expand_parameters)

    elif nt == 'DefaultExpression':
        return "default"
    elif nt == 'Parameter':
        return f"{format_type(e.type)} {e.name}"
    elif nt == 'BoolLiteral':
        return 'true' if e.value else 'false'
    elif nt == 'StringLiteral':
        return f'"{e.value}"'
    elif nt == 'TypeNameExpression':
        return format_expr(e.typeName.type_ref)
    elif nt == 'Mux':
        return f"({format_expr(e.e0)}?{format_expr(e.e1)}:{format_expr(e.e2)})"
    elif nt == 'Slice':
        return f'({format_type_mask(e.type)}({format_expr(e.e0)} >> {format_expr(e.e2)}))'
    elif nt == 'Concat':
        return f'(({format_expr(e.left)} << {e.right.type.size}) | {format_expr(e.right)})'
    elif nt == 'PathExpression':
        name = e.path.name
        is_local = is_control_local_var(name)
        is_abs = expand_parameters and not e.path.absolute
        return f"local_vars->{name}" if is_local else f"parameters.{name}" if is_abs else ""
    elif nt == 'Argument':
        return format_expr(e.expression)
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


def gen_format_call_extern(mcall, mname, mret_type, method_params):
    with types(get_method_call_env(mcall, mname)):
        fmt_params = format_method_parameters(mcall.arguments, method_params)
        all_params = ", ".join([p for p in [fmt_params, "SHORT_STDPARAMS_IN"] if p != ''])

        return_type = format_type(mret_type)
        param_types = ", ".join([format_type(tpar) for (par, tpar) in method_parameters_by_type(mcall.arguments, method_params)] + ["SHORT_STDPARAMS"])

        #pre[ extern ${format_type(mcall.type)} ${mname}(${param_types});
        #[ ${mname}($all_params)


def fld_infos(e):
    for fld in e.arguments[1].expression.components:
        fe = fld.expression
        fee = fe.expr
        breakpoint()
        if 'ref' not in fee or fee.ref.urtype('is_metadata', False):
            hdrname = fee.ref.name
        else:
            hdrname = fee.member
        yield hdrname, fe.member, fe.type.size


def gen_format_call_digest(e):
    var = generate_var_name('digest')
    name = e.typeArguments['Type_Name'][0].path.name
    receiver = e.arguments[0].expression.value

    #pre[ #ifdef T4P4S_NO_CONTROL_PLANE
    #pre[ #error "Generating digest when T4P4S_NO_CONTROL_PLANE is defined"
    #pre[ #endif

    #pre[ debug("    " T4LIT(<,outgoing) " " T4LIT(Sending digest,outgoing) " to port " T4LIT(%d,port) "\n", ${e.arguments[0].expression.value});
    for hdrname, fldname, size in fld_infos(e):
        #pre[ dbg_bytes(field_desc(pd, FLD(${hdrname},${fldname})).byte_addr, (${size}+7)/8, "        : "T4LIT(${fldname},field)"/"T4LIT(${size})" = ");

    #pre[ ctrl_plane_digest $var = create_digest(bg, "$name");
    for hdrname, fldname, size in fld_infos(e):
        #pre[ add_digest_field($var, field_desc(pd, FLD(${hdrname},${fldname})).byte_addr, $size);

    #[ send_digest(bg, $var, $receiver);
    #[ sleep_millis(300);

################################################################################

def format_declaration(d, varname_override = None):
    with SugarStyle("no_comment"):
        return gen_format_declaration(d, varname_override)

# TODO use the variable_name argument in all cases where a variable declaration is created
def format_type(t, variable_name = None, resolve_names = True, addon = ""):
    with SugarStyle("inline_comment"):
        import inspect; import pprint; pprint.PrettyPrinter(indent=4,width=999,compact=True).pprint([f"codegen.sugar.py@{inspect.getframeinfo(inspect.currentframe()).lineno}", t])
        result = gen_format_type(t, resolve_names, variable_name is not None, addon).strip()

        if variable_name is None:
            return result

        split = result.split(" ")
        essential_portion = 2 if split[0] in ['enum', 'struct'] else 1
        return "{} {}{}".format(" ".join(split[0:essential_portion]), variable_name, " ".join(split[essential_portion:]))

def format_method_parameters(ps, mt):
    with SugarStyle("inline_comment"):
        return gen_format_method_parameters(ps, mt)

def format_expr(e, format_as_value=True, expand_parameters=False):
    with SugarStyle("inline_comment"):
        return gen_format_expr(e, format_as_value, expand_parameters)

def format_statement(stmt, ctl=None):
    if ctl is not None:
        compiler_common.enclosing_control = ctl

    compiler_common.pre_statement_buffer = ""
    compiler_common.post_statement_buffer = ""

    ret = gen_format_statement(stmt)

    pre_statement_buffer_ret = compiler_common.pre_statement_buffer
    compiler_common.pre_statement_buffer = ""
    post_statement_buffer_ret = compiler_common.post_statement_buffer
    compiler_common.post_statement_buffer = ""
    return pre_statement_buffer_ret + ret + post_statement_buffer_ret


def format_type_mask(t):
    with SugarStyle("inline_comment"):
        return gen_format_type_mask(t)

def gen_var_name(item, prefix = None):
    if not prefix:
        prefix = f"value_{item.node_type}"
    #[ ${prefix}_${item.id}
