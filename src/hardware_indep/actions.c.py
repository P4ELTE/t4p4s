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
from p4_hlir.hlir.p4_headers import p4_field, p4_field_list, p4_header_keywords
from p4_hlir.hlir.p4_imperatives import p4_signature_ref
from utils.misc import addError, addWarning 
from utils.hlir import *
import math

#[ #include "dpdk_lib.h"
#[ #include "actions.h"
#[ #include <unistd.h>
#[ #include <arpa/inet.h>

#[ extern ctrl_plane_backend bg;


# =============================================================================
# Helpers for field access and update
# (read/write the cached/pre-parsed value where possible)

# TODO now that these abstractions have been made, we might decide to
#   get rid of _AUTO versions of macros and implement the branching here

def modify_int32_int32(f):
    generated_code = ""
    if parsed_field(hlir, f):
        #[ pd->fields.${fld_id(f)} = value32;
        #[ pd->fields.attr_${fld_id(f)} = MODIFIED;
    else:
        #[ MODIFY_INT32_INT32_AUTO_PACKET(pd, ${hdr_fld_id(f)}, value32)
    return generated_code

def extract_int32(f, var, mask = None):
    generated_code = ""
    if parsed_field(hlir, f):
        if mask:
            #[ ${var} = pd->fields.${fld_id(f)} ${mask};
        else:
            #[ ${var} = pd->fields.${fld_id(f)};
        #[ pd->fields.attr_${fld_id(f)} = MODIFIED;
    else:
        #[ EXTRACT_INT32_AUTO_PACKET(pd, ${hdr_fld_id(f)}, ${var});
        if mask:
             #[ ${var} = ${var}${mask};
    return generated_code

# =============================================================================
# Helpers for saturating in add_to_field

def max_value(bitwidth, signed):
    if signed:
        return long(math.pow(2,bitwidth-1)) - 1
    else:
        return long(math.pow(2,bitwidth)) - 1

def min_value(bitwidth, signed):
    if signed:
        return -long(math.pow(2,bitwidth-1)) + 1
    else:
        return 0

# dst += src;
def add_with_saturating(dst, src, bitwidth, signed):
    generated_code = ""
    upper = max_value(bitwidth, signed)
    lower = min_value(bitwidth, signed)
    #[ if (${upper} - ${dst} < ${src}) ${dst} = ${upper};
    #[ else if (${lower} - ${dst} > ${src}) ${dst} = ${lower};
    #[ else ${dst} += ${src};
    return generated_code

# =============================================================================
# Helpers for big numbers

buff = 0

# *valuebuff = value-in-width
def write_int_to_bytebuff(value, width):
    global buff
    generated_code = ""
    l = int_to_big_endian_byte_array_with_length(value, width)
    #[ uint8_t buffer_${buff}[${len(l)}] = {
    for c in l:
        #[     ${c},
    #[ };
    buff = buff + 1
    return generated_code

# =============================================================================
# MASK_GEN

def modify_field_mask( mask ):
    generated_code = ""
    mask_code = ""
    if isinstance(mask, int):
        mask_code = '0x%s' % format(mask,'x')
    elif isinstance(mask, p4_field):
        if mask.width <= 32:
            #[ EXTRACT_INT32_BITS_PACKET(pd, ${hdr_fld_id(mask)}, mask32)
            mask_code = 'mask32'
        else:
            addError("generating modify_field_mask", "Modify field mask is not supported.")
    elif isinstance(mask, p4_signature_ref):
        p = "parameters.%s" % str(fun.signature[mask.idx])
        l = fun.signature_widths[mask.idx]
        if l <= 32:
            #[ mask32 = *${p};
            mask_code = 'mask32'
        else:
            addError("generating modify_field_mask", "Modify field mask is not supported.")
    else:
        addError("generating modify_field_mask", "Modify field mask cannot be recognized.")
    return (generated_code, mask_code)

# =============================================================================
# MODIFY_FIELD

def modify_field(fun, call):
    generated_code = ""
    args = call[1]
    dst = args[0]
    src = args[1]
    mask = ''
    if len(args)==3:
       (gc,m) = modify_field_mask( args[2] )
       #[ ${gc}
       mask = ' & %s' % m
    if not isinstance(dst, p4_field):
        addError("generating modify_field", "We do not allow changing an R-REF yet")
    if isinstance(src, int):
        if not is_vwf(dst) and dst.width <= 32:
            #[ value32 = ${src}${mask};
            #[ ${ modify_int32_int32(dst) }
        else:
            if is_field_byte_aligned(dst):
                #[ ${ write_int_to_bytebuff(src, field_max_width(dst)/8) }
                if is_vwf(dst):
                    #[ MODIFY_BYTEBUF_BYTEBUF_PACKET(pd, ${hdr_fld_id(dst)}, buffer_${buff-1}+(${field_max_width(dst)/8}-field_desc(pd, ${fld_id(dst)}).bytewidth), field_desc(pd, ${fld_id(dst)}).bytewidth)
                else:
                    #[ MODIFY_BYTEBUF_BYTEBUF_PACKET(pd, ${hdr_fld_id(dst)}, buffer_${buff-1}, ${dst.width/8})
            else:
                if is_vwf(dst):
                    addError("generating modify_field", "Modifying non byte-wide variable width field '" + str(dst) + "' with int is not supported")
                else:
                    addError("generating modify_field", "Improper bytebufs cannot be modified yet.")
    elif isinstance(src, p4_field):
        if not is_vwf(dst) and not is_vwf(src) and dst.width <= 32 and src.width <= 32:
            if src.instance.metadata == dst.instance.metadata:
                #[ EXTRACT_INT32_BITS_PACKET(pd, ${hdr_fld_id(src)}, value32)
                #[ MODIFY_INT32_INT32_BITS_PACKET(pd, ${hdr_fld_id(dst)}, value32${mask})
            else:
                #[ ${ extract_int32(src, 'value32', mask) }
                #[ ${ modify_int32_int32(dst) }
        else:
            if is_field_byte_aligned(dst) and is_field_byte_aligned(src) and src.instance.metadata == dst.instance.metadata:
                if mask: # TODO: Mask handling is missing
                    addError("generating modify_field", "Using masks in modify_field on fields longer than 4 bytes is not supported")
                src_fd = "field_desc(pd, " + fld_id(src) + ")"
                dst_fd = "field_desc(pd, " + fld_id(dst) + ")"
                #[ if(${src_fd}.bytewidth < ${dst_fd}.bytewidth) {
                #[     MODIFY_BYTEBUF_BYTEBUF_PACKET(pd, ${hdr_fld_id(dst)}, ${src_fd}.byte_addr, ${src_fd}.bytewidth);
                #[ } else {
                #[     MODIFY_BYTEBUF_BYTEBUF_PACKET(pd, ${hdr_fld_id(dst)}, ${src_fd}.byte_addr + (${src_fd}.bytewidth - ${dst_fd}.bytewidth), ${dst_fd}.bytewidth);
                #[ }
            else:
                if is_vwf(dst):
                    addError("generating modify_field", "Modifying field '" + str(dst) + "' with field '" + str(src) + "' (one of which is a non byte-wide variable width field) is not supported")
                else:
                    addError("generating modify_field", "Improper bytebufs cannot be modified yet.")
    elif isinstance(src, p4_signature_ref):
        p = "parameters.%s" % str(fun.signature[src.idx])
        l = fun.signature_widths[src.idx]
        # TODO: Mask handling
        if not is_vwf(dst) and dst.width <= 32 and l <= 32:
            #[ MODIFY_INT32_BYTEBUF_PACKET(pd, ${hdr_fld_id(dst)}, ${p}, ${(l+7)/8})
        else:
            if is_field_byte_aligned(dst) and l % 8 == 0: #and dst.instance.metadata:
                dst_fd = "field_desc(pd, " + fld_id(dst) + ")"
                #[ if(${l/8} < ${dst_fd}.bytewidth) {
                #[     MODIFY_BYTEBUF_BYTEBUF_PACKET(pd, ${hdr_fld_id(dst)}, ${p}, ${l/8});                
                #[ } else {
                #[     MODIFY_BYTEBUF_BYTEBUF_PACKET(pd, ${hdr_fld_id(dst)}, ${p} + (${l/8} - ${dst_fd}.bytewidth), ${dst_fd}.bytewidth)
                #[ }
            else:
                if is_vwf(dst):
                    addError("generating modify_field", "Modifying non byte-wide variable width field '" + str(src) + "' with p4_signature_ref is not supported")
                else:
                    addError("generating modify_field", "Improper bytebufs cannot be modified yet.")        
    return generated_code

# =============================================================================
# ADD_TO_FIELD

def add_to_field(fun, call):
    generated_code = ""
    args = call[1]
    dst = args[0]
    val = args[1]
    if not isinstance(dst, p4_field):
        addError("generating add_to_field", "We do not allow changing an R-REF yet")
    if isinstance(val, int):
        #[ value32 = ${val};
        if not is_vwf(dst) and dst.width <= 32:
            #[ ${ extract_int32(dst, 'res32') }
            if (p4_header_keywords.saturating in dst.attributes):
               #[ ${ add_with_saturating('value32', 'res32', dst.width, (p4_header_keywords.signed in dst.attributes)) }
            else:
                #[ value32 += res32;
            #[ ${ modify_int32_int32(dst) }
        else:
            if is_vwf(dst):
                addError("generating add_to_field", "add_to_field on variable width field '" + str(dst) + "' is not supported yet")
            else:
                addError("generating add_to_field", "add_to_field on bytebufs (with int) is not supported yet (field: " + str(dst) + ")")
    elif isinstance(val, p4_field):
        if not is_vwf(val) and not is_vwf(dst) and dst.width <= 32 and val.width <= 32:
            #[ ${ extract_int32(val, 'value32') }
            #[ ${ extract_int32(dst, 'res32') }
            if (p4_header_keywords.saturating in dst.attributes):
               #[ ${ add_with_saturating('value32', 'res32', dst.width, (p4_header_keywords.signed in dst.attributes)) }
            else:
                #[ value32 += res32;
            #[ ${ modify_int32_int32(dst) }
        else:
            if is_vwf(val) or is_vwf(dst):
                addError("generating add_to_field", "add_to_field on field '" + str(dst) + "' with field '" + str(val) + "' is not supported yet. One of the fields is a variable width field!")
            else:
                addError("generating add_to_field", "add_to_field on/with bytebufs is not supported yet (fields: " + str(val) + ", " + str(dst) + ")")
    elif isinstance(val, p4_signature_ref):
        p = "parameters.%s" % str(fun.signature[val.idx])
        l = fun.signature_widths[val.idx]
        if not is_vwf(dst) and dst.width <= 32 and l <= 32:
            #[ ${ extract_int32(dst, 'res32') }
            #[ TODO
        else:
            if is_vwf(dst):
                addError("generating add_to_field", "add_to_field on variable width field '" + str(dst) + "' with p4_signature_ref is not supported yet")
            else:
                addError("generating add_to_field", "add_to_field on bytebufs (with p4_signature_ref) is not supported yet (field: " + str(dst) + ")")
    return generated_code

# =============================================================================
# COUNT

def count(fun, call):
    generated_code = ""
    args = call[1]
    counter = args[0]
    index = args[1]
    if isinstance(index, int): # TODO
        #[ value32 = ${index};
    elif isinstance(index, p4_field): # TODO
        #[ ${ extract_int32(index, 'value32') }
    elif isinstance(val, p4_signature_ref):
        #[ value32 = TODO;
    #[ increase_counter(COUNTER_${counter.name}, value32);
    return generated_code

# =============================================================================
# REGISTER_READ

rc = 0

def register_read(fun, call):
    global rc
    generated_code = ""
    args = call[1]
    dst = args[0] # field
    register = args[1]
    index = args[2]
    if isinstance(index, int): # TODO
        #[ value32 = ${index};
    elif isinstance(index, p4_field): # TODO
        #[ ${ extract_int32(index, 'value32') }
    elif isinstance(val, p4_signature_ref):
        #[ value32 = TODO;
    if (register.width+7)/8 < 4:
        #[ uint8_t register_value_${rc}[4];
    else:
        #[ uint8_t register_value_${rc}[${(register.width+7)/8}];
    #[ read_register(REGISTER_${register.name}, value32, register_value_${rc});
    if not is_vwf(dst) and dst.width <= 32:
        #[ memcpy(&value32, register_value_${rc}, 4);
        #[ ${ modify_int32_int32(dst) }
    else:
        if is_field_byte_aligned(dst) and register.width % 8 == 0:
            dst_fd = "field_desc(pd, " + fld_id(dst) + ")"
            reg_bw = register.width / 8
            #[ if(${reg_bw} < ${dst_fd}.bytewidth) {
            #[     MODIFY_BYTEBUF_BYTEBUF_PACKET(pd, ${hdr_fld_id(dst)}, register_value_${rc}, ${reg_bw});
            #[ } else {
            #[     MODIFY_BYTEBUF_BYTEBUF_PACKET(pd, ${hdr_fld_id(dst)}, register_value_${rc} + (${reg_bw} - ${dst_fd}.bytewidth), ${dst_fd}.bytewidth);
            #[ }
        else:
            addError("generating register_read", "Improper bytebufs cannot be modified yet.")
    rc = rc + 1
    return generated_code

# =============================================================================
# REGISTER_WRITE

def register_write(fun, call):
    global rc
    generated_code = ""
    args = call[1]
    register = args[0] # field
    index = args[1]
    src = args[2]
    if isinstance(index, int): # TODO
        #[ res32 = ${index};
    elif isinstance(index, p4_field): # TODO
        #[ ${ extract_int32(index, 'res32') }
    elif isinstance(val, p4_signature_ref):
        #[ res32 = TODO;
    if (register.width+7)/8 < 4:
        #[ uint8_t register_value_${rc}[4];
    else:
        #[ uint8_t register_value_${rc}[${(register.width+7)/8}];
    if isinstance(src, int):
        #[ value32 = ${src};
        #[ memcpy(register_value_${rc}, &value32, 4);
    elif isinstance(src, p4_field):
        if is_vwf(src):
            addError("generating register_write", "Variable width field '" + str(src) + "' in register_write is not supported yet")
        elif register.width <= 32 and src.width <= 32:
            #[ ${ extract_int32(src, 'value32') }
            #[ memcpy(register_value_${rc}, &value32, 4);
        else:
            if src.width == register.width:
                if src.width % 8 == 0 and src.offset % 8 == 0: # and src.instance.metadata == dst.instance.metadata:
                    #[ EXTRACT_BYTEBUF_PACKET(pd, ${hdr_fld_id(src)}, register_value_${rc})
                else:
                    addError("generating register_write", "Improper bytebufs cannot be modified yet.")
            else:
                addError("generating register_write", "Write register-to-field of different widths is not supported yet.")
    #[ write_register(REGISTER_${register.name}, res32, register_value_${rc});
    rc = rc + 1
    return generated_code

# =============================================================================
# GENERATE_DIGEST

def generate_digest(fun, call):
    generated_code = ""
    
    ## TODO make this proper
    extracted_params = []
    for p in call[1]:
        if isinstance(p, int):
            extracted_params += "0" #[str(p)]
        elif isinstance(p, p4_field_list):
            field_list = p
            extracted_params += ["&fields"]
        else:
            addError("generating actions.c", "Unhandled parameter type in generate_digest: " + str(p))
    fun_params = ["bg"] + ["\""+field_list.name+"\""] + extracted_params
    #[  struct type_field_list fields;
    quan = str(len(field_list.fields))
    #[    fields.fields_quantity = ${quan};
    #[    fields.field_offsets = malloc(sizeof(uint8_t*)*fields.fields_quantity);
    #[    fields.field_widths = malloc(sizeof(uint8_t*)*fields.fields_quantity);
    for i,field in enumerate(field_list.fields):
        j = str(i)
        if isinstance(field, p4_field):
            #[    fields.field_offsets[${j}] = (uint8_t*) field_desc(pd, ${fld_id(field)}).byte_addr;
            #[    fields.field_widths[${j}]  =            field_desc(pd, ${fld_id(field)}).bitwidth;
        else:
            addError("generating actions.c", "Unhandled parameter type in field_list: " + name + ", " + str(field))

    params = ",".join(fun_params)

    #[    generate_digest(${params}); sleep(1);
    return generated_code

# =============================================================================
# DROP

def drop(fun, call):
    generated_code = ""
    #[ debug("    :: SETTING PACKET TO BE DROPPED\n");
    #[ pd->dropped=1;
    return generated_code;

# =============================================================================
# RESUBMIT

def resubmit(fun, call):
    generated_code = ""
    #[ debug("    :: RESUBMITTING PACKET\n");
    #[ handle_packet(pd, tables);
    return generated_code;

# =============================================================================
# NO_OP

def no_op(fun, call):
    return "no_op(); // no_op"

# =============================================================================
# PUSH

def push(fun, call):
    generated_code = ""
    args = call[1]
    i = args[0]
    #[ push(pd, header_stack_${i.base_name});
    return generated_code

# =============================================================================
# POP

def pop(fun, call):
    generated_code = ""
    args = call[1]
    i = args[0]
    #[ pop(pd, header_stack_${i.base_name});
    return generated_code

# =============================================================================

for fun in userActions(hlir):
    hasParam = fun.signature
    modifiers = ""
    ret_val_type = "void"
    name = fun.name
    params = ", struct action_%s_params parameters" % (name) if hasParam else ""
    #[ ${modifiers} ${ret_val_type} action_code_${name}(packet_descriptor_t* pd, lookup_table_t** tables ${params}) {
    #[     uint32_t value32, res32, mask32;
    #[     (void)value32; (void)res32; (void)mask32;
    for i,call in enumerate(fun.call_sequence):
        name = call[0].name 

        # Generates a primitive action call to `name'
        if name in locals().keys():
            #[ ${locals()[name](fun, call)}
        else:
            addWarning("generating actions.c", "Unhandled primitive function: " +  name)

    #[ }

