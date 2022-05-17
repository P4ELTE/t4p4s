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
    vw_size = hdr.urtype.vw_fld.urtype.size if vw else 0

    #[     // header ${hdr.name}
    #{     {
    #[         .idx = ${idx},
    #[         .name = "${hdr.name}",
    #[         .byte_width = ${byte_width}, // ${size} bits, ${size/8.0} bytes
    #[         .byte_offset = ${"+".join(byte_offsets)},
    #[         .is_metadata = ${to_c_bool('is_metadata' in hdrt and hdrt.is_metadata)},

    #[         .var_width_field = ${vwfld_name},
    #[         .var_width_size  = ${vw_size},
    if len(flds) == 0:
        #[         // TODO set .first_fld so that it cannot cause any problems
        #[         // TODO set .last_fld so that it cannot cause any problems
    else:
        #[         .first_fld = FLD(${hdr.name},${flds[0].name}),
        #[         .last_fld = FLD(${hdr.name},${flds[-1].name}),
    #}     },
    #[

    byte_offsets += [f'{byte_width}']
    fldidx += len(flds) + vw_size

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

            byte_size = (fld.urtype.size + 7) // 8
            padded_size = byte_size * 8
            binary_txt = '_'.join(f'{mask:0{padded_size}b}'[i:i+8] for i in range(0, padded_size, 8))
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


field_size_print_limit = 12

#{ char* sprintf_hdr_general(char* out, int field_count, uint32_t vals[], uint8_t* ptrs[], int offsets[], int sizes[], int vw_sizes[], const char*const fld_short_names[]) {
#{     #ifdef T4P4S_DEBUG
#{         for (int idx = 0; idx < field_count; ++idx) {
#[             const char* sep_space = idx != field_count - 1 ? " " : "";
#{             if (sizes[idx] <= 32 && vw_sizes[idx] == NO_VW_FIELD_PRESENT) {
#[                 bool is_aligned = sizes[idx] % 8 == 0 && offsets[idx] % 8 == 0;
#[                 bool fld_is_too_large = sizes[idx] < 32 && vals[idx] > 1 << sizes[idx];
#[                 const char* fld_is_too_large_txt = fld_is_too_large ? T4LIT(!too large!,error) : "";
#{                 if (vals[idx] > 9) {
#[                     const char* fmt8 = "." T4LIT(%s,field) "/" T4LIT(%d) "%s=%s" T4LIT(%d) "=0x" T4LIT(%02x,bytes) "%s";
#[                     const char* fmt16 = "." T4LIT(%s,field) "/" T4LIT(%d) "%s=%s" T4LIT(%d) "=0x" T4LIT(%04x,bytes) "%s";
#[                     const char* fmt32 = "." T4LIT(%s,field) "/" T4LIT(%d) "%s=%s" T4LIT(%d) "=0x" T4LIT(%08x,bytes) "%s";
#[                     out += sprintf(out, sizes[idx] > 16 ? fmt32 : sizes[idx] > 8 ? fmt16 : fmt8,
#[                                    fld_short_names[idx],
#[                                    sizes[idx] / (is_aligned ? 8 : 1), is_aligned ? "B" : "b",
#[                                    fld_is_too_large_txt, vals[idx], vals[idx], sep_space);
#[                 } else {
#[                     out += sprintf(out, "." T4LIT(%s,field) "/" T4LIT(%d) "%s=%s" T4LIT(%d) "%s",
#[                                    fld_short_names[idx],
#[                                    sizes[idx] / (is_aligned ? 8 : 1), is_aligned ? "B" : "b",
#[                                    fld_is_too_large_txt, vals[idx], sep_space);
#}                 }
#[             } else {
#[                 int size = vw_sizes[idx] != NO_VW_FIELD_PRESENT ? vw_sizes[idx] : sizes[idx];
#[                 out += sprintf(out, "." T4LIT(%s,field) "/%s" T4LIT(%d) "%s=" T4COLOR(T4LIGHT_bytes),
#[                                fld_short_names[idx],
#[                                ${to_c_bool(fld.is_vw)} ? "vw" : "", size / (size % 8 == 0 ? 8 : 1), size % 8 == 0 ? "B" : "b");
#[                 out += dbg_sprint_bytes_limit(out, ptrs[idx], size/8, ${field_size_print_limit}, "_");
#[                 out += sprintf(out, T4COLOR(T4LIGHT_off) "%s", sep_space);
#}             }
#}         }
#}     #endif
#[     return out;
#} }
#[


for hdr in unique_everseen(hlir.header_instances):
    #{ const char* detailed_sprintf_hdr_${hdr.name}(char* out, packet_descriptor_t* pd, header_descriptor_t* hdr) {
    field_count = len(hdr.urtype.fields)
    #[     uint32_t vals[${field_count}];
    #[     uint8_t* ptrs[${field_count}];
    sizes = ", ".join(f'{fld.size}' for fld in hdr.urtype.fields)
    offsets = ", ".join(f'{fld.offset}' for fld in hdr.urtype.fields)
    vw_sizes = ", ".join('NO_VW_FIELD_PRESENT' if not fld.is_vw else f'hdr->vw_size' for fld in hdr.urtype.fields)
    names = ", ".join(f'"{fld.short_name}"' for fld in hdr.urtype.fields)
    #[     int offsets[${field_count}] = { $offsets };
    #[     int sizes[${field_count}] = { $sizes };
    #[     int vw_sizes[${field_count}] = { ${vw_sizes} };
    #[     const char*const fld_short_names[${field_count}] = { $names };
    for idx, fld in enumerate(hdr.urtype.fields):
        if fld.size <= 32 and not fld.is_vw:
            #[     vals[$idx] = GET32(src_pkt(pd), FLD(${hdr.name}, ${fld.name}));  // ${hdr.name}.${fld.name}/${fld.size}b
        else:
            #[     ptrs[$idx] = hdr->pointer + fld_infos[FLD(${hdr.name},${fld.name})].byte_offset;  // ${hdr.name}.${fld.name}/${(fld.size+7)//8}B
    #[     return sprintf_hdr_general(out, ${field_count}, vals, ptrs, offsets, sizes, vw_sizes, fld_short_names);
    #} }
    #[


#{ const char* sprintf_hdr(char* out, packet_descriptor_t* pd, header_descriptor_t* hdr) {
#{     #ifdef T4P4S_DEBUG
#[         const char* name = hdr_infos[hdr->type].name;
for hdr in unique_everseen(hlir.header_instances):
    field_count = len(hdr.urtype.fields)
    #[         if (!strcmp("${hdr.name}", name))    return detailed_sprintf_hdr_${hdr.name}(out, pd, hdr);
#}     #endif
#[     return NULL; // should never happen
#} }
#[


#{ int get_fld_vw_size(field_instance_e fld, packet_descriptor_t* pd) {
#[     header_instance_e hdr = fld_infos[fld].header_instance;
#[     return pd->headers[hdr].vw_size;
#} }
#[
