# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from compiler_log_warnings_errors import addError, addWarning
from utils.codegen import format_expr, format_type, format_statement, format_declaration, to_c_bool
from compiler_common import statement_buffer_value, generate_var_name, get_hdr_name, unique_everseen

import functools


#[ #include "dpdk_lib.h"
#[ #include "util_packet.h"
#[ #include "gen_include.h"
#[ #include "hdr_fld.h"
#[ #include "hdr_fld_sprintf.h"


#{ const char* header_instance_names[HEADER_COUNT] = {
for hdr in hlir.header_instances:
    #[     "${hdr.name}",
#} };
#[

#{ const char* field_names[FIELD_COUNT] = {
for hdr in hlir.header_instances:
    #[     // ${hdr.name}
    for fld in hdr.urtype.fields:
        #[         "${fld.short_name}", // ${hdr.name}.${fld.name}
#} };
#[


#{ const hdr_info_t hdr_infos[HEADER_COUNT] = {
byte_offsets = ["0"]
fldidx = 0
for idx, hdr in enumerate(hlir.header_instances):
    hdrt = hdr.urtype
    flds = hdrt.fields

    size = hdrt.size if 'size' in hdrt else 0
    byte_width = hdrt('byte_width', 0)

    vw = list(hdr.urtype.fields.filter(lambda fld: fld('is_vw', False)))
    vwfld_name = f'FLD({hdr.name},{vw[0].name})' if vw else 'NO_VW_FIELD_PRESENT'

    #[     // header ${hdr.name}
    #{     {
    #[         .idx = ${idx},
    #[         .name = "${hdr.name}",
    #[         .byte_width = ${byte_width}, // ${size} bits, ${size/8.0} bytes
    #[         .byte_offset = ${"+".join(byte_offsets)},
    #[         .is_metadata = ${to_c_bool('is_metadata' in hdrt and hdrt.is_metadata)},

    #[         .var_width_field = ${vwfld_name},
    if len(flds) == 0:
        #[         // TODO set .first_fld so that it cannot cause any problems
        #[         // TODO set .last_fld so that it cannot cause any problems
    else:
        #[         .first_fld = FLD(${hdr.name},${flds[0].name}),
        #[         .last_fld = FLD(${hdr.name},${flds[-1].name}),
    #}     },
    #[

    byte_offsets += [f'{byte_width}']
    fldidx += len(flds)

if len(hlir.header_instances) == 0:
    #[ {}, // dummy
#} };


#{ const fld_info_t fld_infos[FIELD_COUNT] = {
hdr_startidxs = {}
fldidx = 0
for hdr in hlir.header_instances:
    for fld in hdr.urtype.fields:
        fldidx += 1
        if hdr.name not in hdr_startidxs:
            hdr_startidxs[hdr.name] = fldidx

        is_meta = hdr.urtype.is_metadata

        #[     // field ${hdr.name}.${fld.name}
        #{     {
        #[         .header_instance = HDR(${'all_metadatas' if is_meta else hdr.name}),
        #[         .size = ${fld.urtype.size},
        #[         .byte_width = to_bytes(${fld.urtype.size}),
        #[         .bit_offset = ${fld.offset} % 8,
        #[         .byte_offset = ${fld.offset} / 8,
        #[         .is_vw = ${to_c_bool(fld.is_vw)},
        if fld.urtype.size <= 32:
            not0 = 0xffffffff

            if is_meta:
                mask = not0 >> (32 - fld.urtype.size)
                mask_txt = f'0x{mask:08x}'
            else:
                shift_up = (32 - fld.urtype.size) % 32
                top_bits = (not0 << shift_up) & not0
                mask = top_bits >> (fld.offset % 8)
                mask_txt = f'0x{mask:08x}'

            binary_txt = '_'.join(f'{mask:032b}'[i:i+8] for i in range(0, 32, 8))
            #[         .mask = ${mask_txt}, // ${fld.urtype.size}b at offset ${fld.offset//8}B+${fld.offset%8}b: 0b${binary_txt}
        else:
            #[         // .mask ignored: ${fld.urtype.size}b field is restricted to be byte aligned (over 32b)
        #}     },
        #[
#} };


#{ const stk_info_t stk_infos[STACK_COUNT] = {
for stk in hlir.header_stacks:
    stk0 = f'{stk.name}_0'
    #[     // stack ${stk.name}
    #{     {
    #[         .size      = ${stk.urtype.stk_size.value},
    #[         .fld_count = ${len(stk.type.elementType.urtype.fields)},
    #[         .start_hdr = HDR(${stk0}),
    #[         .start_fld = ${hdr_startidxs[stk0]},
    #}     },
    #[
#} };
#[




#{ const char* sprintf_hdr(char* out, packet_descriptor_t* pd, header_descriptor_t* hdr) {
#{     #ifdef T4P4S_DEBUG
#[         const char* name = hdr_infos[hdr->type].name;
for hdr in unique_everseen(hlir.header_instances):
    #[         if (!strcmp("${hdr.name}", name))    return sprintf_hdr_${hdr.name}(out, pd, hdr);
#}     #endif
#[     return NULL; // should never happen
#} }
#[


for hdr in unique_everseen(hlir.header_instances):
    field_size_print_limit = 12
    #{ const char* sprintf_hdr_${hdr.name}(char* out, packet_descriptor_t* pd, header_descriptor_t* hdr) {
    #{     #ifdef T4P4S_DEBUG
    #[         char* ptr = out;
    #[         int idx = 0;
    #[         uint8_t* hdrptr = pd->headers[HDR(${hdr.name})].pointer;
    for fld in hdr.urtype.fields:
        name = fld.name
        size_fmt = f'T4LIT({fld.size//8}) "B"' if fld.size%8 == 0 else f'T4LIT({fld.size}) "b"'
        if fld.size <= 32 and not fld.is_vw:
            is_aligned = fld.size%8 == 0 and fld.offset%8 == 0
            #[         uint32_t val_$name = GET32(src_pkt(pd), FLD(${hdr.name},$name));
            #[         bool ${name}_is_too_large = ${fld.size} < 32 && val_$name > 1 << ${fld.size};
            #[         const char* ${name}_is_too_large_txt = ${name}_is_too_large ? T4LIT(!too large!,error) : "";
            #{         if (val_$name > 9) {
            #[             idx += sprintf(ptr + idx, "." T4LIT(${fld.short_name},field) "/" T4LIT(%d) "%s=%s" T4LIT(%d) "=0x" T4LIT(%0${fld.size//4}x,bytes) " ",
            #[                            ${fld.size // (8 if is_aligned else 1)}, ${to_c_bool(is_aligned)} ? "B" : "b",
            #[                            ${name}_is_too_large_txt, val_$name, val_$name);
            #[         } else {
            #[             idx += sprintf(ptr + idx, "." T4LIT(${fld.short_name},field) "/" T4LIT(%d) "%s=%s" T4LIT(%d) " ",
            #[                            ${fld.size // (8 if is_aligned else 1)}, ${to_c_bool(is_aligned)} ? "B" : "b",
            #[                            ${name}_is_too_large_txt, val_$name);
            #}         }
        else:
            #[         field_instance_e fld_$name = FLD(${hdr.name},${fld.name});
            #[         int size_$name = fld_infos[fld_$name].is_vw ? pd->headers[hdr->type].vw_size : ${fld.size};
            #[         idx += sprintf(ptr + idx, "." T4LIT(${fld.short_name},field) "/%s" T4LIT(%d) "%s=" T4COLOR(T4LIGHT_bytes),
            #[                        ${to_c_bool(fld.is_vw)} ? "vw" : "", size_$name / (size_$name % 8 == 0 ? 8 : 1), size_$name % 8 == 0 ? "B" : "b");
            #[         idx += dbg_sprint_bytes_limit(ptr + idx, hdrptr + fld_infos[FLD(${hdr.name},$name)].byte_offset, size_$name/8, ${field_size_print_limit}, "_");
            #[         idx += sprintf(ptr + idx, T4COLOR(T4LIGHT_off) " ");

    #}     #endif
    #[     return out;
    #} }
    #[


#{ int get_fld_vw_size(field_instance_e fld, packet_descriptor_t* pd) {
#[     header_instance_e hdr = fld_infos[fld].header_instance;
#[     return pd->headers[hdr].vw_size;
#} }
#[
