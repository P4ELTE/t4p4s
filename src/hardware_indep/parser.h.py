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
    stack_instances = filter(lambda i: i.max_index > 0 and not i.virtual, hlir.p4_headers.values())
    hs_names = list(set(map(lambda i: i.base_name, stack_instances)))
    xs = map(lambda s : "{" + ",\n  ".join(instances4stack(hlir, s)) + "}", hs_names)
    return hsarrayarray("header_instance_t", xs)

# TODO remove after implemented in hlir16
def header_stack_size():
    stack_instances = filter(lambda i: i.max_index > 0 and not i.virtual, hlir.p4_headers.values())
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
#[   header_stack_,
#[ };



#[ // Header instance infos
#[ // ---------------------

#[ #define HEADER_INSTANCE_COUNT ${len(hlir16.header_instances)}


#[ enum header_instance_e {
for hdr in hlir16.header_instances:
    #[   header_instance_${hdr.name},
#[ };


#[ static const int header_instance_byte_width[HEADER_INSTANCE_COUNT] = {
for hdr in hlir16.header_instances:
    #[   ${hdr.type.byte_width}, // header_instance_${hdr.type.inst_name}, ${hdr.type.bit_width} bits, ${hdr.type.bit_width/8.0} bytes
#[ };


#[ static const int header_instance_is_metadata[HEADER_INSTANCE_COUNT] = {
for hdr in hlir16.header_instances:
    #[   ${1 if hdr.type.is_metadata else 0}, // header_instance_${hdr.type.inst_name}
#[ };


#[ // Note: -1: is fixed-width field
#[ static const int header_instance_var_width_field[HEADER_INSTANCE_COUNT] = {
for hdr in hlir16.header_instances:
    #[   ${1 if hdr.type.is_vw else -1}, // header_instance_${hdr.type.inst_name}
#[ };


# TODO move to hlir16.py/set_additional_attrs?
def all_field_instances():
    return [fld for hdr in hlir16.header_instances for fld in hdr.type.fields]


#[ // Field instance infos
#[ // --------------------


#[ #define FIELD_INSTANCE_COUNT ${len(all_field_instances())}


#[ typedef enum header_instance_e header_instance_t;
#[ typedef enum field_instance_e field_instance_t;


#[ enum field_instance_e {
for hdr in hlir16.header_instances:
    for fld in hdr.type.fields:
        #[   field_instance_${hdr.type.inst_name}_${fld.name},
#[ };



#[ static const int field_instance_bit_width[FIELD_INSTANCE_COUNT] = {
for hdr in hlir16.header_instances:
    for fld in hdr.type.fields:
        #[   ${fld.type.size}, // field_instance_${hdr.type.name}_${fld.name}
#[ };


#[ static const int field_instance_bit_offset[FIELD_INSTANCE_COUNT] = {
for hdr in hlir16.header_instances:
    for fld in hdr.type.fields:
        #[   (${fld.offset} % 8), // field_instance_${hdr.type.name}_${fld.name}
#[ };


# TODO why does this name have "_hdr" at the end, but field_instance_bit_offset doesn't?

#[ static const int field_instance_byte_offset_hdr[FIELD_INSTANCE_COUNT] = {
for hdr in hlir16.header_instances:
    for fld in hdr.type.fields:
        #[   (${fld.offset} / 8), // field_instance_${hdr.type.name}_${fld.name}
#[ };



#[ // TODO documentation
#[ #define mod_top(n, bits) (((bits-(n%bits)) % bits))

#[ // Produces a 32 bit int that has n bits on at the top end.
#[ #define uint32_top_bits(n) (0xffffffff << mod_top(n, 32))

#[ static const int field_instance_mask[FIELD_INSTANCE_COUNT] = {
for hdr in hlir16.header_instances:
    for fld in hdr.type.fields:
        #[  __bswap_constant_32(uint32_top_bits(${fld.type.size}) >> (${fld.offset}%8)), // field_instance_${hdr.type.name}_${fld.name},
#[ };


#[ static const header_instance_t field_instance_header[FIELD_INSTANCE_COUNT] = {
for hdr in hlir16.header_instances:
    for fld in hdr.type.fields:
        #[   header_instance_${hdr.type.inst_name}, // field_instance_${hdr.type.name}_${fld.name}
#[ };


# TODO convert to hlir16 properly
# #[ ${ header_stack_elements() }
#[ static const header_instance_t header_stack_elements[HEADER_STACK_COUNT][10] = {
#[     // TODO
#[ };

# TODO convert to hlir16 properly
# #[ ${ header_stack_size() }
#[ static const unsigned header_stack_size[HEADER_STACK_COUNT] = {
#[     // TODO
#[ };

#[ typedef enum header_stack_e header_stack_t;


#[ #endif // __HEADER_INFO_H__
