#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# SPDX-License-Identifier: Apache-2.0
# Copyright 2016 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_type

#[ #pragma once

#[ #include <byteswap.h>
#[ #include <stdbool.h>
#[ #include "aliases.h"
#[ #include "hdr_fld.h"
#[ #include "common_enums.h"


#[ #define to_bytes(bits) (((bits) + 7) / 8)
#[

#[ // TODO documentation
#[ #define mod_top(n, bits) (((bits-(n%bits)) % bits))

#[ // Produces a 32 bit int that has n bits on at the top end.
#[ #define uint32_top_bits(n) (0xffffffff << mod_top(n, 32))

#[ // the type has some optional values 
#[ #define MAYBE(type,...)    type


#[ #define NO_VW_FIELD_PRESENT (-1)


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
for hdr, fld in all_fields:
    fldtype = fld.type.type if fld.type.node_type == 'StructField' else fld.type
    varname = f'FLD({hdr.name},{fld._expression.name})'
    #[     ${format_type(fldtype, varname=varname)};
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

# note: implemented in hdr_fld.c.py
#[ extern const char* field_names[FIELD_COUNT];
#[ extern const char* header_instance_names[HEADER_COUNT];

# TODO maybe some more space needs to be added on for varlen headers?
nonmeta_hdrlens = "+".join([f'{hdr.urtype.byte_width}' for hdr in hlir.header_instances])
#[ #define NONMETA_HDR_TOTAL_LENGTH ($nonmeta_hdrlens)


#{ typedef enum {
for hdr in hlir.header_instances:
    #[     HDR(${hdr.name}),
if len(hlir.header_instances) == 0:
    #[ HDR(__dummy__),
#} } header_instance_e;
#[

#{ typedef enum {
for hdr, fld in all_fields:
    #[   FLD(${hdr.name},${fld.name}),
if len(all_fields) == 0:
    #[     FLD(__dummy__,__dummy__),
#} } field_instance_e;
#[

#{ typedef enum {
for stk in hlir.header_stacks:
    #[     STK(${stk.name}),
if len(hlir.header_stacks) == 0:
    #[     STK(__dummy__),
#} } header_stack_e;
#[


#{ typedef struct {
#[     const int        idx;
#[     const char*const name;

#[     const int        byte_width;
#[     const int        byte_offset;

#[     const bool       is_metadata;

#[     const field_instance_e first_fld;
#[     const field_instance_e last_fld;

#[     const MAYBE(field_instance_e, NO_VW_FIELD_PRESENT) var_width_field;
#} } hdr_info_t;
#[

#{ typedef struct {
#[     const int               size;
#[     const int               bit_offset;
#[     const int               byte_width;
#[     const int               byte_offset;
#[     const uint32_t          mask;
#[     const bool              is_metadata;
#[     const bool              is_vw;
#[     const header_instance_e header_instance;
#} } fld_info_t;
#[

#{ typedef struct {
#[     const int               size;
#[     const int               fld_count;
#[     const header_instance_e start_hdr;
#[     const field_instance_e  start_fld;
#} } stk_info_t;
#[


# note: implemented in hdr_fld.c.py
#[ extern const hdr_info_t hdr_infos[HEADER_COUNT];
#[ extern const fld_info_t fld_infos[FIELD_COUNT];
#[ extern const stk_info_t stk_infos[STACK_COUNT];


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
            #[     ${loc.urtype.name}_t ${loc.name};
        else:
            #[     ${format_type(loc.type, addon=loc.name)}; // type: ${loc.urtype.name}
    else:
        #[     ${format_type(loc.type, addon=loc.name)};

for name in vw_names:
    #[     uint8_t ${name}_var; // Width of the variable width field // type: ${name}

if len(parser.parserLocals) + len(vw_names) == 0:
    #[     // no parser locals
#} } parser_state_t;


for stk in hlir.header_stacks:
    for idx, fld in enumerate(stk.urtype.elementType.urtype.fields):
        #[ #define stkfld_offset_${stk.name}_${fld.name} $idx


#[ #define PARSED_AFTER_END_OF_PACKET INT_MIN
#[ #define PARSED_OVER_STACK_SIZE     (INT_MIN+1)
#[ #define HDR_PARSED_OK              0
