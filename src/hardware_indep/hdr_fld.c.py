# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from compiler_log_warnings_errors import addError, addWarning
from utils.codegen import format_expr, format_type, format_statement, format_declaration
from compiler_common import statement_buffer_value, generate_var_name, get_hdr_name

import functools


#[ #include "dpdk_lib.h"
#[ #include "util_packet.h"
#[ #include "gen_include.h"


#{ const char* header_instance_names[HEADER_COUNT] = {
for hdr in hlir.header_instances:
    #[     "${hdr.name}",
#} };
#[

#{ const char* field_names[FIELD_COUNT] = {
for hdr in hlir.header_instances:
    #[     // ${hdr.name}
    for fld in hdr.urtype.fields:
        #[         "${fld.name}", // ${hdr.name}.${fld.name}
#} };
#[


#{ const hdr_info_t hdr_infos[HEADER_COUNT] = {
byte_offsets = ["0"]
for idx, hdr in enumerate(hlir.header_instances):
    typ = hdr.urtype
    typ_bit_width = typ.bit_width if 'bit_width' in typ else 0
    typ_byte_width = typ.byte_width if 'byte_width' in typ else 0

    #[     // header ${hdr.name}
    #{     {
    #[         .idx = ${idx},
    #[         .name = "${hdr.name}",
    #[         .byte_width = ${typ_byte_width}, // ${typ_bit_width} bits, ${typ_bit_width/8.0} bytes
    #[         .byte_offset = ${"+".join(byte_offsets)},
    #[         .is_metadata = ${'true' if 'is_metadata' in typ and typ.is_metadata else 'false'},
    #[         .var_width_field = ${functools.reduce((lambda x, f: f.id if hasattr(f, 'is_vw') and f.is_vw else x), hdr.urtype.fields, 'FIXED_WIDTH_FIELD')},
    #}     },
    #[

    byte_offsets += [f'{typ_byte_width}']

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

        not0 = 0xffffffff
        shift_up = (32 - fld.urtype.size) % 32
        top_bits = (not0 << shift_up) & not0
        mask = top_bits >> (fld.offset % 8)
        mask_txt = f'{mask:08x}'
        binary_txt = '_'.join(f'{mask:032b}'[i:i+8] for i in range(0, 32, 8))

        #[     // field ${hdr.name}.${fld.name}
        #{     {
        #[         .byte_width = (${fld.urtype.size} + 7) / 8,
        #[         .bit_width = ${fld.urtype.size},
        #[         .bit_offset = ${fld.offset} % 8,
        #[         .byte_offset = ${fld.offset} / 8,
        #[         .mask = __bswap_constant_32(0x${mask_txt}), // ${fld.urtype.size} bits at offset ${fld.offset}: ${binary_txt}
        #[         .is_metadata = ${'true' if hdr.urtype.is_metadata else 'false'},
        #[         .header_instance = HDR(${'all_metadatas' if hdr.urtype.is_metadata else hdr.name}),
        #}     },
        #[
#} };


#{ const stk_info_t stk_infos[STACK_COUNT] = {
for stk in hlir.header_stacks:
    stk0 = f'{stk.name}_0'
    #[     // stack ${stk.name}
    #{     {
    #[         .size      = ${stk.urtype.size.value},
    #[         .fld_count = ${len(stk.type.elementType.urtype.fields)},
    #[         .start_hdr = HDR(${stk.name}_0),
    #[         .start_fld = ${hdr_startidxs[stk0]},
    #}     },
    #[
#} };
