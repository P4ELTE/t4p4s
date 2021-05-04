#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

import functools

#[ #pragma once

#[ #include <byteswap.h>
#[ #include <stdbool.h>
#[ #include "aliases.h"

#{ typedef enum {
#[     NOT_MODIFIED,
#[     MODIFIED,
#} } parsed_field_attr_t;

all_fields = [(hdr, fld) for hdr in hlir.header_instances for fld in hdr.urtype.fields]
parsed_fields = [(hdr, fld) for hdr, fld in all_fields if not fld.preparsed]

for hdr in hlir.header_instances:
    if hdr.urtype.node_type == 'Type_HeaderUnion':
        raise NotImplementedError("Header unions are not supported")

#{ typedef struct {
for hdr, fld in parsed_fields:
    #[     uint32_t FLD(${hdr.name},${fld._expression.name});
#[

for hdr, fld in parsed_fields:
    #[     parsed_field_attr_t FLD_ATTR(${hdr.name},${fld._expression.name});
#[
#} } parsed_fields_t;


#[ // Header instance infos
#[ // ---------------------

#[ #define HEADER_COUNT ${max(len(hlir.header_instances), 1)}
#[ #define FIELD_COUNT ${max(len(all_fields), 1)}
#[ #define STACK_COUNT ${max(len(hlir.header_stacks), 1)}

#[ extern const char* field_names[FIELD_COUNT];
#[ extern const char* header_instance_names[HEADER_COUNT];

# TODO maybe some more space needs to be added on for varlen headers?
nonmeta_hdrlens = "+".join([f'{hdr.urtype.byte_width}' for hdr in hlir.header_instances])
#[ #define NONMETA_HDR_TOTAL_LENGTH ($nonmeta_hdrlens)


#[ #define FIXED_WIDTH_FIELD (-1)


#{ typedef enum {
for hdr in hlir.header_instances:
    #[     HDR(${hdr.name}),
if len(hlir.header_instances) == 0:
    #[ HDR(__dummy__),
#} } header_instance_t;
#[

#{ typedef enum {
for hdr in hlir.header_instances:
    for fld in hdr.urtype.fields:
        #[   FLD(${hdr.name},${fld.name}),
if len(hlir.header_instances) == 0:
    #[ FLD(__dummy__,__dummy__),
#} } field_instance_t;
#[

#{ typedef enum {
for stk in hlir.header_stacks:
    #[     STK(${stk.name}),
if len(hlir.header_stacks) == 0:
    #[ STK(__dummy__),
#} } header_stack_t;
#[

#[ // TODO documentation
#[ #define mod_top(n, bits) (((bits-(n%bits)) % bits))

#[ // Produces a 32 bit int that has n bits on at the top end.
#[ #define uint32_top_bits(n) (0xffffffff << mod_top(n, 32))


#{ typedef struct {
#[     int         idx;
#[     const char* name;
#[     int         byte_width;
#[     int         byte_offset;
#[     bool        is_metadata;
#[     int         var_width_field;
#} } hdr_info_t;
#[

#{ typedef struct {
#[     int               bit_width;
#[     int               bit_offset;
#[     int               byte_width;
#[     int               byte_offset;
#[     uint32_t          mask;
#[     bool              is_metadata;
#[     header_instance_t header_instance;
#} } fld_info_t;
#[

#{ typedef struct {
#[     int               size;
#[     int               fld_count;
#[     header_instance_t start_hdr;
#[     field_instance_t  start_fld_idx;
#} } stk_info_t;
#[



#{ static const hdr_info_t hdr_infos[HEADER_COUNT] = {
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


#{ static const fld_info_t fld_infos[FIELD_COUNT] = {
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
        #[         .byte_width = ${hdr.urtype.byte_width},
        #[         .is_metadata = ${'true' if hdr.urtype.is_metadata else 'false'},
        #[         .bit_width = ${fld.urtype.size},
        #[         .bit_offset = ${fld.offset} % 8,
        #[         .byte_offset = ${fld.offset} / 8,
        #[         .mask = __bswap_constant_32(0x${mask_txt}), // ${fld.urtype.size} bits at offset ${fld.offset}: ${binary_txt}
        #[         // .mask = __bswap_constant_32(uint32_top_bits(${fld.urtype.size}) >> (${fld.offset}%8)),
        #[         .header_instance = HDR(${'all_metadatas' if hdr.urtype.is_metadata else hdr.name}),
        #}     },
        #[
#} };


#{ static const stk_info_t stk_infos[STACK_COUNT] = {
for stk in hlir.header_stacks:
    stk0 = f'{stk.name}_0'
    #[     // stack ${stk.name}
    #{     {
    #[         .size      = ${stk.urtype.size.value},
    #[         .fld_count = ${len(stk.type.elementType.urtype.fields)},
    #[         .start_hdr = HDR(${stk.name}_0),
    #[         .start_fld_idx = ${hdr_startidxs[stk0]},
    #}     },
    #[
#} };


for stk in hlir.header_stacks:
    for idx, fld in enumerate(stk.urtype.elementType.urtype.fields):
        #[ #define stkfld_offset_${stk.name}_${fld.name} $idx


#[ // HW optimization related infos
#[ // --------------------

#[ #define OFFLOAD_CHECKSUM ${'true' if []!=[x for x in hlir.sc_annotations if x.name=='offload'] else 'false'}


#[ // Parser state local vars
#[ // -----------------------

parser = hlir.parsers[0]

vw_names = [hdr.name for hdr in hlir.header_instances.filter(lambda hdr: not hdr.urtype.is_metadata and hdr.urtype.is_vw)]

#{ typedef struct {
for loc in parser.parserLocals:
    if 'type_ref' in loc.type:
        if loc.urtype.node_type == 'Type_Extern':
            #[ ${loc.urtype.name}_t ${loc.name};
        else:
            #[ uint8_t ${loc.name}[${loc.urtype.byte_width}]; // type: ${loc.urtype.name}
    else:
        #[ uint8_t ${loc.name}[(${loc.type.size}+7)/8];

for name in vw_names:
    #[ uint8_t ${name}_var; // Width of the variable width field // type: ${name}

if len(parser.parserLocals) + len(vw_names) == 0:
    #[     // no parser locals
#} } parser_state_t;
