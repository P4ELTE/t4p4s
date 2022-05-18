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
#[                 bool is_vw = vw_sizes[idx] != NO_VW_FIELD_PRESENT;
#[                 int size = is_vw ? vw_sizes[idx] : sizes[idx];
#[                 out += sprintf(out, "." T4LIT(%s,field) "/%s" T4LIT(%d) "%s=" T4COLOR(T4LIGHT_bytes),
#[                                fld_short_names[idx],
#[                                is_vw ? "vw" : "", size / (size % 8 == 0 ? 8 : 1), size % 8 == 0 ? "B" : "b");
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
