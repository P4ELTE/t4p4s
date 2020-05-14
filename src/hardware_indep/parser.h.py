# Copyright 2016 Eotvos Lorand University, Budapest, Hungary  Licensed under
# the Apache License, Version 2.0 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of the License
# at  http://www.apache.org/licenses/LICENSE-2.0  Unless required by
# applicable law or agreed to in writing, software distributed under the
# License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
# OF ANY KIND, either express or implied. See the License for the specific
# language governing permissions and limitations under the License.
from hlir16.utils_hlir16 import *


#[ #ifndef __HEADER_INFO_H__
#[ #define __HEADER_INFO_H__

#[ #include <byteswap.h>
#[ #include <stdbool.h>

#[ // TODO add documentation
#[ #define MODIFIED true

# TODO put this in a proper header
#[ typedef struct {} InternetChecksum_t;


#{ typedef struct parsed_fields_s {

for hdr in hlir16.header_instances_with_refs:
    if hdr.type.type_ref.node_type == 'Type_HeaderUnion':
        raise NotImplementedError("Header unions are not supported")

    for fld in hdr.type.type_ref.fields:
        fld = fld._expression
        fldtype = fld.canonical_type()

        if fldtype.size <= 32:
            #[ uint32_t field_instance_${hdr.name}_${fld.name};
            #[ uint8_t attr_field_instance_${hdr.name}_${fld.name};
#} } parsed_fields_t;


#[ // Header stack infos
#[ // ------------------


# TODO make proper header stacks (types grouped by instances? order?)

#[ #define HEADER_STACK_COUNT ${len(hlir16.header_instances_with_refs)}


#{ enum header_stack_e {
for hi in hlir16.header_instances_with_refs:
    #[ header_stack_${hi.name},
#[ header_stack_, // dummy to prevent warning for empty stack
#} };



#[ // Header instance infos
#[ // ---------------------

#[ #define HEADER_INSTANCE_COUNT ${len(hlir16.header_instances)}

# TODO maybe some more needs to be added for varlen headers?
nonmeta_hdrlens = "+".join([str(hi.type.type_ref.byte_width) for hi in hlir16.header_instances_with_refs if not hi.canonical_type().is_metadata])
#[ #define HEADER_INSTANCE_TOTAL_LENGTH ($nonmeta_hdrlens)


#{ struct header_instance_info {
#[     const char* name;
#[     int         byte_width;
#[     int         byte_offset;
#[     bool        is_metadata;
#} };

#[ extern const char* header_instance_names[HEADER_INSTANCE_COUNT];

#{ typedef enum header_instance_e {
for hdr in hlir16.header_instances:
    if not hdr.type._type_ref.is_metadata:
        #[ header_instance_${hdr.name},

#[ header_instance_all_metadatas,
#} } header_instance_t;


#{ static const struct header_instance_info header_instance_infos[HEADER_INSTANCE_COUNT+1] = {
byte_offsets = []
for hdr in hlir16.header_instances:
    typ = hdr.type.type_ref if hasattr(hdr.type, 'type_ref') else hdr.type
    typ_bit_width = typ.bit_width if hasattr(typ, 'bit_width') else 0
    typ_byte_width = typ.byte_width if hasattr(typ, 'byte_width') else 0

    #{ { // header_instance_${hdr.name}
    #[     "${hdr.name}",
    #[     ${typ_byte_width}, // header_instance_${hdr.name}, ${typ_bit_width} bits, ${typ_bit_width/8.0} bytes
    #[     ${"+".join(byte_offsets) if byte_offsets != [] else "0"},
    #[     ${'true' if hasattr(typ, 'is_metadata') and typ.is_metadata else 'false'},
    #} },

    byte_offsets += [str(typ_byte_width)]

#[ { // dummy
#[ },
#} };


#{ static const int header_instance_byte_width[HEADER_INSTANCE_COUNT] = {
for hdr in hlir16.header_instances_with_refs:
    #[   ${hdr.type.type_ref.byte_width}, // header_instance_${hdr.name}, ${hdr.type.type_ref.bit_width} bits, ${hdr.type.type_ref.bit_width/8.0} bytes
#} };


#{ static const int header_instance_byte_width_summed[HEADER_INSTANCE_COUNT+1] = {
#[     0,
byte_widths = []
for hdr in hlir16.header_instances_with_refs:
    byte_widths += [str(hdr.type.type_ref.byte_width)]
    joined = "+".join(byte_widths)
    #[   $joined,
#} };


#{ static const int header_instance_is_metadata[HEADER_INSTANCE_COUNT] = {
for hdr in hlir16.header_instances_with_refs:
    #[   ${1 if hdr.type.type_ref.is_metadata else 0}, // header_instance_${hdr.name}
#} };


# TODO move to hlir16.py/set_additional_attrs?
def all_field_instances():
    return [fld for hdr in hlir16.header_instances_with_refs for fld in hdr.type.type_ref.fields]


#[ // Field instance infos
#[ // --------------------


#[ #define FIELD_INSTANCE_COUNT ${len(all_field_instances())}


#{ typedef enum field_instance_e {
for hdr in hlir16.header_instances_with_refs:
    for fld in hdr.type.type_ref.fields:
        #[   field_instance_${hdr.name}_${fld.name},
#} } field_instance_t;


#[ #define FIXED_WIDTH_FIELD -1
#{ static const int header_instance_var_width_field[HEADER_INSTANCE_COUNT] = {
for hdr in hlir16.header_instances_with_refs:
    field_id_pattern = 'field_instance_{}_{}'
    #[   ${reduce((lambda x, f: field_id_pattern.format(hdr.name, f.name) if hasattr(f, 'is_vw') and f.is_vw else x), hdr.type.type_ref.fields, 'FIXED_WIDTH_FIELD')}, // header_instance_${hdr.name}
#} };


#{ static const int field_instance_bit_width[FIELD_INSTANCE_COUNT] = {
for hdr in hlir16.header_instances_with_refs:
    for fld in hdr.type.type_ref.fields:
        fldtype = fld.canonical_type()
        #[   ${fldtype.size}, // field_instance_${hdr.name}_${fld.name}
#} };


#{ static const int field_instance_bit_offset[FIELD_INSTANCE_COUNT] = {
for hdr in hlir16.header_instances_with_refs:
    for fld in hdr.type.type_ref.fields:
        #[   (${fld.offset} % 8), // field_instance_${hdr.name}_${fld.name}
#} };


# TODO why does this name have "_hdr" at the end, but field_instance_bit_offset doesn't?

#{ static const int field_instance_byte_offset_hdr[FIELD_INSTANCE_COUNT] = {
for hdr in hlir16.header_instances_with_refs:
    for fld in hdr.type.type_ref.fields:
        #[   (${fld.offset} / 8), // field_instance_${hdr.name}_${fld.name}
#} };



#[ // TODO documentation
#[ #define mod_top(n, bits) (((bits-(n%bits)) % bits))

#[ // Produces a 32 bit int that has n bits on at the top end.
#[ #define uint32_top_bits(n) (0xffffffff << mod_top(n, 32))

#{ static const int field_instance_mask[FIELD_INSTANCE_COUNT] = {
for hdr in hlir16.header_instances_with_refs:
    for fld in hdr.type.type_ref.fields:
        fldtype = fld.canonical_type()
        #[  __bswap_constant_32(uint32_top_bits(${fldtype.size}) >> (${fld.offset}%8)), // field_instance_${hdr.name}_${fld.name},
#} };


#{ static const header_instance_t field_instance_header[FIELD_INSTANCE_COUNT] = {
for hdr in hlir16.header_instances_with_refs:
    for fld in hdr.type.type_ref.fields:
        if hdr.type.type_ref.is_metadata:
            #[ header_instance_all_metadatas,
        else:
            #[ header_instance_${hdr.name}, // field_instance_${hdr.name}_${fld.name}
#} };


#[ // TODO current stacks are exactly 1 element deep 
#{ static const header_instance_t header_stack_elements[HEADER_STACK_COUNT][10] = {
for hi in hlir16.header_instances_with_refs:
    #[ // header_instance_${hi.name}
    #{ {
    for stack_elem in [hi.name]:
        if hdr.type.type_ref.is_metadata:
            #[ header_instance_all_metadatas,
        else:
            #[ header_instance_${stack_elem},
    #} },
#} };

#{ static const unsigned header_stack_size[HEADER_STACK_COUNT] = {
for hi in hlir16.header_instances_with_refs:
    #[ 1, // ${hi.name}
#} };

#[ typedef enum header_stack_e header_stack_t;

#[ /////////////////////////////////////////////////////////////////////////////
#[ // HEADER TYPE AND FIELD TYPE INFORMATION
#[ // TODO remove instance info when no code refers to it

#[ #define HEADER_COUNT ${len(hlir16.header_types)}

#{ enum header_e {
for hdr in hlir16.header_types:
    #[   header_${hdr.name},
#} };

#{ static const int header_byte_width[HEADER_COUNT] = {
for hdr in hlir16.header_types:
    #[   ${hdr.byte_width}, // ${hdr.name}
#} };

#{ static const int header_is_metadata[HEADER_COUNT] = {
for hdr in hlir16.header_types:
    #[   ${1 if hdr.is_metadata else 0}, // ${hdr.name}
#} };


def all_fields():
    return [fld for hdr in hlir16.header_types for fld in hdr.fields]

#[ #define FIELD_COUNT ${len(all_fields())}

#[ extern const char* field_names[FIELD_COUNT];

#[ typedef enum header_e header_t;
#[ typedef enum field_e field_t;

#{ enum field_e {
for hdr in hlir16.header_types:
    for fld in hdr.fields:
        #[   field_${hdr.name}_${fld.name},
#} };

#{ static const int field_bit_width[FIELD_COUNT] = {
for hdr in hlir16.header_types:
    for fld in hdr.fields:
        fldtype = fld.canonical_type()
        #[ ${fldtype.size}, // field_${hdr.name}_${fld.name}
#} };

#{ static const int field_bit_offset[FIELD_COUNT] = {
for hdr in hlir16.header_types:
    for fld in hdr.fields:
        #[   (${fld.offset} % 8), // field_${hdr.name}_${fld.name}
#} };

#{ static const int field_byte_offset_hdr[FIELD_COUNT] = {
for hdr in hlir16.header_types:
    for fld in hdr.fields:
        #[   (${fld.offset} / 8), // field_${hdr.name}_${fld.name}
#} };

#[ #define mod_top(n, bits) (((bits-(n%bits)) % bits))
#[ // Produces a 32 bit int that has n bits on at the top end.
#[ #define uint32_top_bits(n) (0xffffffff << mod_top(n, 32))

#{ static const int field_mask[FIELD_COUNT] = {
for hdr in hlir16.header_types:
    for fld in hdr.fields:
        fldtype = fld.canonical_type()
        #[  __bswap_constant_32(uint32_top_bits(${fldtype.size}) >> (${fld.offset}%8)), // field_${hdr.name}_${fld.name},
#} };

#{ static const header_t field_header[FIELD_COUNT] = {
for hdr in hlir16.header_types:
    for fld in hdr.fields:
        #[   header_${hdr.name}, // field_${hdr.name}_${fld.name}
#} };

#{ static const int header_var_width_field[HEADER_COUNT] = {
for hdr in hlir16.header_types:
    #[   ${reduce((lambda x, f: f.id if hasattr(f, 'is_vw') and f.is_vw else x), hdr.fields, 'FIXED_WIDTH_FIELD')}, // ${hdr.name}
#} };


for enum in hlir16.objects['Type_Enum']:
    #[ enum ${enum.c_name} { ${', '.join([m.c_name for m in enum.members])} };

for error in hlir16.objects['Type_Error']:
    #[ enum ${error.c_name} { ${', '.join([m.c_name for m in error.members])} };

#[
#[ // HW optimization related infos
#[ // --------------------
#[ #define OFFLOAD_CHECKSUM ${'true' if []!=[x for x in hlir16.sc_annotations if x.name=='offload'] else 'false'}


#[ // Parser state local vars
#[ // -----------------------

parser = hlir16.objects['P4Parser'][0]

#{ typedef struct parser_state_s {
for loc in parser.parserLocals:
    if hasattr(loc.type, 'type_ref'):
        if loc.type.type_ref.node_type == 'Type_Extern':
            #[ ${loc.type.type_ref.name}_t ${loc.name};
        else:
            #[ uint8_t ${loc.name}[${loc.type.type_ref.byte_width}]; // type: ${loc.type.type_ref.name}
    else:
        #[ uint8_t ${loc.name}[(${loc.type.size}+7)/8];
    #[ uint8_t ${loc.name}_var; // Width of the variable width field // type: ${loc.type.type_ref.name}
#} } parser_state_t;


#[ #endif // __HEADER_INFO_H__
