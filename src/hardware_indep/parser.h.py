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
import inspect
from utils.hlir import *
from hlir16.utils_hlir16 import *

# TODO remove once dependent functions are implemented
def hsarray(t, es):
    es2 = [ a + " // " + b + "" for (a,b) in zip(es, header_stack_ids(hlir))]
    return "static const %s %s[HEADER_STACK_COUNT] = {\n  %s\n};\n\n" % (t, inspect.stack()[1][3], ",\n  ".join(es2))
# TODO remove once dependent functions are implemented
def hsarrayarray(t, es):
    es2 = [ a + " // " + b + "" for (a,b) in zip(es, header_stack_ids(hlir))]
    return "static const %s %s[HEADER_STACK_COUNT][10] = {\n  %s\n};\n\n" % (t, inspect.stack()[1][3], ",\n  ".join(es2))

#==============================================================================

# TODO remove after implemented in hlir16
def header_stack_elements():
    stack_instances = filter(lambda i: i.max_index > 0 and not i.virtual, hlir.p4_header_instances.values())
    hs_names = list(set(map(lambda i: i.base_name, stack_instances)))
    xs = map(lambda s : "{" + ",\n  ".join(instances4stack(hlir, s)) + "}", hs_names)
    return hsarrayarray("header_instance_t", xs)

# TODO remove after implemented in hlir16
def header_stack_size():
    stack_instances = filter(lambda i: i.max_index > 0 and not i.virtual, hlir.p4_header_instances.values())
    hs_names = list(set(map(lambda i: i.base_name, stack_instances)))
    xs = map(lambda s : str(len(instances4stack(hlir, s))), hs_names)
    return hsarray("unsigned", xs)



#[ #ifndef __HEADER_INFO_H__
#[ #define __HEADER_INFO_H__

#[ #include <byteswap.h>

#[ // TODO add documentation
#[ #define MODIFIED 1

#[ typedef struct parsed_fields_s {
# TODO convert to hlir16
for f in hlir.p4_fields.values():
    if parsed_field(hlir, f):
        if f.width <= 32:
            #[ uint32_t ${fld_id(f)};
            #[ uint8_t attr_${fld_id(f)};
#[ } parsed_fields_t;


#[ // Header stack infos
#[ // ------------------


sc = len(header_stack_ids(hlir))

#[ #define HEADER_STACK_COUNT ${sc}


#[ enum header_stack_e {
# TODO convert to hlir16
for hsid in header_stack_ids(hlir):
    #[ ${hsid},
#[   header_stack_
#[ };



#[ // Header instance infos
#[ // ---------------------


#[ #define HEADER_INSTANCE_COUNT ${len(hlir16.metadatas) + len(hlir16.header_instances)}


#[ enum header_instance_e {
for meta in hlir16.metadatas:
    #[   header_instance_${meta.inst_name},
for header_inst in hlir16.header_instances:
    #[   header_instance_${header_inst.name},
#[ };


#[ static const int header_instance_byte_width[HEADER_INSTANCE_COUNT] = {
for meta in hlir16.metadatas:
    meta_len = type_bit_width(hlir16, meta)
    #[   ${bits_to_bytes(meta_len)}, // header_instance_${meta.inst_name}, ${meta_len} bits, ${meta_len/8.0} bytes
for header_inst in hlir16.headers:
    bits = get_bit_width(hlir16, header_inst)
    #[   ${bits_to_bytes(bits)}, // header_instance_${header_inst.name}, ${bits} bits, ${bits/8.0} bytes
#[ };


#[ static const int header_instance_is_metadata[HEADER_INSTANCE_COUNT] = {
for meta in hlir16.metadatas:
    #[   1, // header_instance_standard_metadata
for header_inst in hlir16.headers:
    bits = get_bit_width(hlir16, header_inst)
    #[   0, // header_instance_${header_inst.name}
#[ };


#[ // Note: -1: is fixed-width field
#[ static const int header_instance_var_width_field[HEADER_INSTANCE_COUNT] = {
for meta in hlir16.metadatas:
    #[   -1, // header_instance_${meta.inst_name}
for header_inst in hlir16.header_instances:
    #[   ${get_bit_width(hlir16, header_inst)}, // header_instance_${header_inst.name}
#[ };


def all_field_instances():
    return [(meta.inst_name, meta) for meta in hlir16.metadatas] + [(hdr.name, hdr.header_type) for hdr in hlir16.header_instances]

def all_field_instance_fields():
    return [(fld, hdr_name, hdr_type) for hdr_name, hdr_type in all_field_instances() for fld in hdr_type.fields]


#[ // Field instance infos
#[ // --------------------


#[ #define FIELD_INSTANCE_COUNT ${len(all_field_instance_fields())}


#[ typedef enum header_instance_e header_instance_t;
#[ typedef enum field_instance_e field_instance_t;


#[ enum field_instance_e {
for fld, hdr_name, hdr_type in all_field_instance_fields():
    #[   field_instance_${hdr_name}_${fld.name},
#[ };



#[ static const int field_instance_bit_width[FIELD_INSTANCE_COUNT] = {
for fld, hdr_name, hdr_type in all_field_instance_fields():
    fld_type = get_type(hlir16, fld)
    #[   ${fld_type.size}, // field_instance_${hdr_name}_${fld.name}
#[ };


#[ static const int field_instance_bit_offset[FIELD_INSTANCE_COUNT] = {
for fld, hdr_name, hdr_type in all_field_instance_fields():
    #[   (${fld.offset} % 8), // field_instance_${hdr_name}_${fld.name}
#[ };


# TODO why does this name have "_hdr" at the end, but field_instance_bit_offset doesn't?

#[ static const int field_instance_byte_offset_hdr[FIELD_INSTANCE_COUNT] = {
for fld, hdr_name, hdr_type in all_field_instance_fields():
    #[   (${fld.offset} / 8), // field_instance_${hdr_name}_${fld.name}
#[ };



#[ // TODO documentation
#[ #define mod_top(n, bits) (((bits-(n%bits)) % bits))

#[ // Produces a 32 bit int that has n bits on at the top end.
#[ #define uint32_top_bits(n) (0xffffffff << mod_top(n, 32))

#[ static const int field_instance_mask[FIELD_INSTANCE_COUNT] = {
for fld, hdr_name, hdr_type in all_field_instance_fields():
    fld_type = get_type(hlir16, fld)
    #[  __bswap_constant_32(uint32_top_bits(${fld_type.size}) >> (${fld.offset}%8)), // field_instance_${hdr_name}_${fld.name},
#[ };


#[ static const header_instance_t field_instance_header[FIELD_INSTANCE_COUNT] = {
for hdr_name, hdr_type in all_field_instances():
    for fld in hdr_type.fields:
        #[   header_instance_${hdr_name}, // field_instance_${hdr_name}_${fld.name}
#[ };



# TODO convert to hlir16

#[ ${ header_stack_elements() }
#[ ${ header_stack_size() }
#[ typedef enum header_stack_e header_stack_t;


#[ #endif // __HEADER_INFO_H__
