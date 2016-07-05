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
from p4_hlir.hlir.p4_headers import p4_field, p4_field_list
from p4_hlir.hlir.p4_imperatives import p4_signature_ref
from utils.misc import addError, addWarning 
from utils.hlir import hdr_prefix, fld_prefix, fld_id, userActions 

#[ #include "dpdk_lib.h"
#[ #include "actions.h"
#[ #include <unistd.h>
#[ #include <arpa/inet.h>
#[ extern void increase_counter (int counterid, int index);
#[
#[ extern backend bg;
#[

actions = hlir.p4_actions
useractions = userActions(actions)
useraction_objs = [(actions[act_key]) for act_key in useractions ]

# =============================================================================
# MODIFY_FIELD

def modify_field(fun, call):
    generated_code = ""
    args = call[1]
    dst = args[0]
    src = args[1]
    # mask = args[2]
    if not isinstance(dst, p4_field):
        addError("generating modify_field", "We do not allow changing an R-REF yet")
    if isinstance(src, int):
        #[ value32 = ${src};
        if dst.width <= 32:
            #[ MODIFY_INT32_INT32_AUTO(pd, ${fld_id(dst)}, value32)
        else:
            if dst.width % 8 == 0 and dst.offset % 8 == 0:
                #[ MODIFY_BYTEBUF_INT32(pd, ${fld_id(dst)}, value32) //TODO: This macro is not implemented
            else:
                addError("generating modify_field", "Improper bytebufs cannot be modified yet.")
    elif isinstance(src, p4_field):
        if dst.width <= 32 and src.width <= 32:
            if src.instance.metadata == dst.instance.metadata:
                #[ EXTRACT_INT32_BITS(pd, ${fld_id(src)}, value32)
                #[ MODIFY_INT32_INT32_BITS(pd, ${fld_id(dst)}, value32)
            else:
                #[ EXTRACT_INT32_AUTO(pd, ${fld_id(src)}, value32)
                #[ MODIFY_INT32_INT32_AUTO(pd, ${fld_id(dst)}, value32)
        elif src.width != dst.width:
            addError("generating modify_field", "bytebuf field-to-field of different widths is not supported yet")
        else:
            if dst.width % 8 == 0 and dst.offset % 8 == 0 and src.width % 8 == 0 and src.offset % 8 == 0 and src.instance.metadata == dst.instance.metadata:
                #[ MODIFY_BYTEBUF_BYTEBUF(pd, ${fld_id(dst)}, FIELD_BYTE_ADDR(pd, field_desc(${fld_id(src)})), (field_desc(${fld_id(dst)})).bytewidth)
            else:
                addError("generating modify_field", "Improper bytebufs cannot be modified yet.")
    elif isinstance(src, p4_signature_ref):
        p = "parameters.%s" % str(fun.signature[src.idx])
        l = fun.signature_widths[src.idx]
        if dst.width <= 32 and l <= 32:
            #[ MODIFY_INT32_BYTEBUF(pd, ${fld_id(dst)}, ${p}, ${(l+7)/8})
        else:
            if dst.width % 8 == 0 and dst.offset % 8 == 0 and l % 8 == 0: #and dst.instance.metadata:
                #[ MODIFY_BYTEBUF_BYTEBUF(pd, ${fld_id(dst)}, ${p}, (field_desc(${fld_id(dst)})).bytewidth)
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
        if dst.width <= 32:
            #[ EXTRACT_INT32_AUTO(pd, ${fld_id(dst)}, res32)
            #[ value32 += res32;
            #[ MODIFY_INT32_INT32_AUTO(pd, ${fld_id(dst)}, value32)
        else:
            addError("generating modify_field", "Bytebufs cannot be modified yet.")
    elif isinstance(val, p4_field):
        if dst.width <= 32 and val.length <= 32:
            #[ EXTRACT_INT32_AUTO(pd, ${fld_id(val)}, value32)
            #[ EXTRACT_INT32_AUTO(pd, ${fld_id(dst)}, res32)
            #[ value32 += res32;
            #[ MODIFY_INT32_INT32_AUTO(pd, ${fld_id(dst)}, value32)
        else:
            addError("generating add_to_field", "bytebufs cannot be modified yet.")
    elif isinstance(val, p4_signature_ref):
        p = "parameters.%s" % str(fun.signature[val.idx])
        l = fun.signature_widths[val.idx]
        if dst.width <= 32 and l <= 32:
            #[ EXTRACT_INT32_AUTO(pd, ${fld_id(dst)}, res32)
            #[ TODO
        else:
            addError("generating add_to_field", "bytebufs cannot be modified yet.")
    return generated_code

# =============================================================================
# COUNT

def count(fun, call):
    generated_code = ""
    args = call[1]
    counter = args[0]
    index = args[1]
    if isinstance(index, int): # TODO
        #[ value32 = ${val};
    elif isinstance(index, p4_field): # TODO
        #[ EXTRACT_INT32_AUTO(pd, ${fld_id(index)}, value32)
    elif isinstance(val, p4_signature_ref):
        #[ value32 = TODO;
    #[ increase_counter(COUNTER_${counter.name}, value32);
    return generated_code

# =============================================================================
# GENERATE_DIGEST

def generate_digest(fun, call):
    generated_code = ""
    
    ## TODO make this proper
    fun_params = ["bg", "\"mac_learn_digest\""]
    for p in call[1]:
        if isinstance(p, int):
            fun_params += "0" #[str(p)]
        elif isinstance(p, p4_field_list):
            field_list = p
            fun_params += ["&fields"]
        else:
            addError("generating actions.c", "Unhandled parameter type in generate_digest: " + str(p))
 
    #[  struct type_field_list fields;
    quan = str(len(field_list.fields))
    #[    fields.fields_quantity = ${quan};
    #[    fields.field_offsets = malloc(sizeof(uint8_t*)*fields.fields_quantity);
    #[    fields.field_widths = malloc(sizeof(uint8_t*)*fields.fields_quantity);
    for i,field in enumerate(field_list.fields):
        j = str(i)
        if isinstance(field, p4_field):
            #[    fields.field_offsets[${j}] = (uint8_t*) (pd->headers[header_instance_${field.instance}].pointer + field_instance_byte_offset_hdr[field_instance_${field.instance}_${field.name}]);
            #[    fields.field_widths[${j}] = field_instance_bit_width[field_instance_${field.instance}_${field.name}]*8;
        else:
            addError("generating actions.c", "Unhandled parameter type in field_list: " + name + ", " + str(field))

    params = ",".join(fun_params)
    #[
    #[    generate_digest(${params}); sleep(1);
    return generated_code

# =============================================================================
# DROP

def drop(fun, call):
    return "drop(pd);"

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

for fun in useraction_objs:
    hasParam = fun.signature
    modifiers = ""
    ret_val_type = "void"
    name = fun.name
    params = ", struct action_%s_params parameters" % (name) if hasParam else ""
    #[ ${modifiers} ${ret_val_type} action_code_${name}(packet_descriptor_t* pd, lookup_table_t** tables ${params}) {
    #[     uint32_t value32, res32;
    #[     (void)value32; (void)res32;
    for i,call in enumerate(fun.call_sequence):
        name = call[0].name 

        # Generates a primitive action call to `name'
        if name in locals().keys():
            #[ ${locals()[name](fun, call)}
        else:
            addWarning("generating actions.c", "Unhandled primitive function: " +  name)

    #[ }
    #[

