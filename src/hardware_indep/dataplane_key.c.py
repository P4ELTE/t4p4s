# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_declaration, format_statement, format_expr, format_type, gen_format_type, get_method_call_env
from compiler_log_warnings_errors import addError, addWarning
from compiler_common import types, generate_var_name, get_hdrfld_name, unique_everseen
from hlir16.hlir_utils import align8_16_32

#[ #include "gen_include.h"
#[ #include "dataplane_impl.h"
#[ #include "dpdk_lib_byteorder.h"



# TODO this is too low level

#[ extern int FLD_BYTEOFFSET(header_descriptor_t hdesc, field_instance_e fld);

#{ uint8_t* handle_byte_addr(packet_descriptor_t* pd, field_instance_e fld) {
#[     int hdridx = fld_infos[fld].header_instance;
#[     header_descriptor_t hdr = pd->headers[hdridx];
#[     return (((uint8_t*)hdr.pointer) + (FLD_BYTEOFFSET(hdr, fld)));
#} }
#[

#{ bool is_invalid(packet_descriptor_t* pd, header_instance_e hdr, const char* hi_name, const char* fld_name) {
#{     if (unlikely(!is_header_valid(hdr, pd))) {
#{         #ifdef T4P4S_DEBUG
#[             debug(" " T4LIT(!!!!,error) " " T4LIT(Lookup on invalid header,error) " " T4LIT(%s,header) "." T4LIT(%s,field) ", " T4LIT(it will contain an unspecified value,warning) "\n", hi_name, fld_name);
#}         #endif
#[         return true;
#}     }
#[     return false;
#} }
#[


#[ uint8_t* get_fld_ptr(packet_descriptor_t* pd, field_instance_e fld) {
#[     header_instance_e hdr = fld_infos[fld].header_instance;
#[     return pd->headers[hdr].pointer + fld_infos[fld].byte_offset;
#[ }

field_size_print_limit = 12

for table in hlir.tables:
    if 'key' not in table or table.key_bit_size == 0:
        continue

    #{ void table_${table.name}_key(packet_descriptor_t* pd, uint8_t* key KEYTXTPARAMS) {
    sortedkeys = sorted(table.key.keyElements, key=lambda k: k.match_order)
    #TODO variable length fields
    #TODO field masks
    for key in sortedkeys:
        if 'header' in key:
            hi_name = "all_metadatas" if key.header.urtype.is_metadata else key.header.name

            #[     if (is_invalid(pd, HDR(${hi_name}), "${hi_name}", "${key.field_name}"))    return;

            fld = key.header.urtype.fields.get(key.field_name)

            size = key.size
            byte_width = (size+7)//8
            if size <= 32:
                padded_byte_width = align8_16_32(size)
                #[     *(uint${padded_byte_width}_t*)key = GET32(src_pkt(pd), FLD(${hi_name},${key.field_name}));

                #{     #ifdef T4P4S_DEBUG
                #[         *key_txt_idx += sprintf(key_txt + *key_txt_idx, T4LIT(${hi_name},header) "." T4LIT(${fld.short_name},field) "/" T4LIT(%db) "=" T4LIT(%d) "=" T4COLOR(T4LIGHT_bytes) "0x",
                #[                                 ${size}, net2t4p4s_${padded_byte_width//8}(*(uint${padded_byte_width}_t*)key));
                #[         *key_txt_idx += dbg_sprint_bytes_limit(key_txt + *key_txt_idx, get_fld_ptr(pd, FLD(${hi_name},${key.field_name})), ${byte_width}, ${field_size_print_limit}, "_");
                #[         *key_txt_idx += sprintf(key_txt + *key_txt_idx, T4COLOR(T4LIGHT_off) " ");
                #}     #endif
            else:
                if size % 8 != 0:
                    addWarning("table key computation", f"Field {key.id}/{size}b is longer than 32b and not byte aligned, may cause problems")

                #[     GET_BUF(key, src_pkt(pd), FLD(${key.header.name},${key.field_name}));

                #{     #ifdef T4P4S_DEBUG
                #[         *key_txt_idx += sprintf(key_txt + *key_txt_idx, T4LIT(${hi_name},header) "." T4LIT(${fld.short_name},field) "/" T4LIT(%db) "=" T4COLOR(T4LIGHT_bytes), $size);
                #[         *key_txt_idx += dbg_sprint_bytes_limit(key_txt + *key_txt_idx, get_fld_ptr(pd, FLD(${hi_name},${key.field_name})), ${byte_width}, ${field_size_print_limit}, "_");
                #[         *key_txt_idx += sprintf(key_txt + *key_txt_idx, T4COLOR(T4LIGHT_off) " ");
                #}     #endif

            #[     key += ${byte_width};
        else:
            ke = key.expression
            if ke.node_type == 'MethodCallExpression':
                byte_width = (ke.urtype.size + 7) // 8
                mname = ke.method.member
                var = generate_var_name(f'mcall_{mname}')
                #[     ${format_type(ke.type)} $var = ${format_expr(ke)};
                #[     memcpy(key, &$var, ${byte_width});

                #{     #ifdef T4P4S_DEBUG
                if mname == 'isValid':
                    #[         *key_txt_idx += sprintf(key_txt + *key_txt_idx, "$mname() =" T4LIT(%s,bytes) " ", is_header_valid(HDR(${hi_name}), pd) ? "false" : "true");
                else:
                    #[         *key_txt_idx += sprintf(key_txt + *key_txt_idx, "$mname() =" T4COLOR(T4LIGHT_bytes));
                    #[         *key_txt_idx += dbg_sprint_bytes_limit(key_txt + *key_txt_idx, get_fld_ptr(pd, FLD(${hi_name},${key.field_name})), ${byte_width}, ${field_size_print_limit}, "_");
                    #[         *key_txt_idx += sprintf(key_txt + *key_txt_idx, T4COLOR(T4LIGHT_off) " ");
                #}     #endif
            elif key.size <= 32 or key.size % 8 == 0:
                if ke.node_type == 'Slice':
                    high = ke.e1.value
                    low = ke.e2.value
                    bit_width = high - low + 1
                    byte_width = (bit_width+7) // 8

                    bit_offset = ke.e0.urtype.size - high - 1
                    byte_offset = (bit_offset+7) // 8

                    is_byte_aligned = bit_width % 8 == 0

                    hdrname, fldname = get_hdrfld_name(ke.e0)

                    # TODO this implementation is too low level
                    #[     memcpy(key, handle_byte_addr(pd, FLD($hdrname,$fldname)) + ${byte_offset}, ${byte_width}); // slice: ${hdrname}.${fldname}[$high:$low]

                    #{     #ifdef T4P4S_DEBUG
                    #[         *key_txt_idx += sprintf(key_txt + *key_txt_idx, T4LIT(${hi_name},header) "." T4LIT(${fld.short_name},field) "[" T4LIT($high) ":]" T4LIT($low) "=" T4COLOR(T4LIGHT_bytes));
                    #[         *key_txt_idx += dbg_sprint_bytes_limit(key_txt + *key_txt_idx, key, ${byte_width}, ${field_size_print_limit}, "_");
                    #[         *key_txt_idx += sprintf(key_txt + *key_txt_idx, T4COLOR(T4LIGHT_off) " ");
                    #}     #endif

                    if not is_byte_aligned:
                        in_byte_offset = bit_offset - byte_offset * 8
                        # TODO implement this case properly
                        pass
                else:
                    # fld is a control local
                    byte_width = (key.size+7)//8
                    locname = ke.path.name
                    short_name = table.control.controlLocals.get(locname, 'Declaration_Variable').short_name
                    #[     memcpy(key, &((control_locals_${table.control.name}_t*) pd->control_locals)->${locname}, ${byte_width});

                    #{     #ifdef T4P4S_DEBUG
                    #[         *key_txt_idx += sprintf(key_txt + *key_txt_idx, T4LIT(${short_name},field) "/" T4LIT(%db) "=" T4COLOR(T4LIGHT_bytes), ${key.size});
                    #[         *key_txt_idx += dbg_sprint_bytes_limit(key_txt + *key_txt_idx, key, ${byte_width}, ${field_size_print_limit}, "_");
                    #[         *key_txt_idx += sprintf(key_txt + *key_txt_idx, T4COLOR(T4LIGHT_off) " ");
                    #}     #endif
            else:
                addWarning("table key computation", f"Skipping unsupported key component {ke.path.name} ({key.size} bits): it is over 32 bits long and not byte aligned")
                continue

            #[     key += ${byte_width};
        #[
    #} }
    #[
