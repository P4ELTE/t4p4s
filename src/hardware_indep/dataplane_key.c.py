# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_declaration, format_statement, format_expr, format_type, gen_format_type, get_method_call_env
from compiler_log_warnings_errors import addError, addWarning
from compiler_common import types, generate_var_name, get_hdrfld_name, unique_everseen

#[ #include "dataplane_impl.h"

for table in hlir.tables:
    if 'key' not in table or table.key_length_bits == 0:
        continue

    #{ void table_${table.name}_key(packet_descriptor_t* pd, uint8_t* key) {
    sortedfields = sorted(table.key.keyElements, key=lambda k: k.match_order)
    #TODO variable length fields
    #TODO field masks
    for f in sortedfields:
        if 'header' in f:
            hi_name = "all_metadatas" if f.header.urtype.is_metadata else f.header.name

            #{     if (unlikely(!is_header_valid(HDR(${hi_name}), pd))) {
            #{         #ifdef T4P4S_DEBUG
            #[             debug(" " T4LIT(!!!!,error) " " T4LIT(Lookup on invalid header,error) " " T4LIT(${hi_name},header) "." T4LIT(${f.field_name},field) ", " T4LIT(it will contain an unspecified value,warning) "\n");
            #}         #endif
            #[         return;
            #}     }

            if f.size <= 32:
                #[     EXTRACT_INT32_BITS_PACKET(pd, HDR(${hi_name}), FLD(${f.header.name},${f.field_name}), *(uint32_t*)key);
                #[     key += sizeof(uint32_t);
            elif f.size > 32 and f.size % 8 == 0:
                byte_width = (f.size+7)//8
                #[     EXTRACT_BYTEBUF_PACKET(pd, HDR(${hi_name}), FLD(${f.header.name},${f.field_name}), key);
                #[     key += ${byte_width};
            else:
                addWarning("table key computation", f"Skipping unsupported field {f.id} ({f.size} bits): it is over 32 bits long and not byte aligned")
        else:
            fx = f.expression
            if fx.node_type == 'MethodCallExpression':
                byte_width = (fx.urtype.size + 7) // 8
                var = generate_var_name(f'mcall_{fx.method.member}')
                #[     ${format_type(fx.type)} $var = ${format_expr(fx)};
                #[     memcpy(key, &$var, ${byte_width});
            elif f.size <= 32 or f.size % 8 == 0:
                if fx.node_type == 'Slice':
                    high = fx.e1.value
                    low = fx.e2.value
                    bit_width = high - low + 1
                    byte_width = (bit_width+7) // 8

                    bit_offset = fx.e0.urtype.size - high - 1
                    byte_offset = (bit_offset+7) // 8

                    is_byte_aligned = bit_width % 8 == 0

                    hdrname, fldname = get_hdrfld_name(fx.e0)

                    # TODO this implementation is too low level
                    #[     memcpy(key, handle(header_desc_ins(pd, HDR($hdrname)), FLD($hdrname,$fldname)).byte_addr + ${byte_offset}, ${byte_width}); // slice: ${hdrname}.${fldname}[$high:$low]

                    if not is_byte_aligned:
                        in_byte_offset = bit_offset - byte_offset * 8
                        # TODO implement this case properly
                        pass
                    continue

                # f is a control local
                byte_width = (f.size+7)//8
                fld_name = fx.path.name
                #[     memcpy(key, &((control_locals_${table.control.name}_t*) pd->control_locals)->${fld_name}, ${byte_width});
            else:
                addWarning("table key computation", f"Skipping unsupported key component {fx.path.name} ({f.size} bits): it is over 32 bits long and not byte aligned")
                continue

            #[     key += ${byte_width};
        #[

    if table.matchType.name == "lpm":
        #[     key -= ${table.key_length_bytes};
    #} }
    #[
