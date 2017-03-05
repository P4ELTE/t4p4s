// Copyright 2016 Eotvos Lorand University, Budapest, Hungary
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef DPDK_PRIMITIVES_H
#define DPDK_PRIMITIVES_H

#include <rte_byteorder.h>

// Returns a pointer (uint8_t*) to the byte containing the first bit of the given field in the packet
#define FIELD_BYTE_ADDR(pd, fd) (fd.fixed_pos ? (((uint8_t*)(pd)->headers[fd.header].pointer)+fd.byteoffset) :\
        (((uint8_t*)(pd)->headers[fd.header].pointer)+(fd.byteoffset + (pd)->headers[fd.header].var_width_field_excess_byte_count)))

#define FIELD_BITWIDTH(pd, field) (field_desc(field).fixed_width ? field_desc(field).bitwidth : (pd)->headers[field_desc(field).header].var_width_field_bitwidth)
#define FIELD_BITCOUNT(pd, field) (FIELD_BITWIDTH(pd, field) + field_desc(field).bitoffset)

#define FIELD_BYTEWIDTH(pd, field) ((FIELD_BITWIDTH(pd, field) + 7) / 8)
#define FIELD_BYTECOUNT(pd, field) ((FIELD_BITCOUNT(pd, field) + 7) / 8)

#define FIELD_MASK(pd, field) (field_desc(field).fixed_width ? field_desc(field).mask : \
    rte_cpu_to_be_32((0xffffffff << (32 - FIELD_BITCOUNT(pd, field))) & (0xffffffff >> field_desc(field).bitoffset)))

#define FIELD_BYTES(pd, field) ( \
     FIELD_BYTECOUNT(pd, field) == 1 ? (*(uint8_t*) FIELD_BYTE_ADDR(pd, field_desc(field))) : \
    (FIELD_BYTECOUNT(pd, field) == 2 ? (*(uint16_t*)FIELD_BYTE_ADDR(pd, field_desc(field))) : \
                                       (*(uint32_t*)FIELD_BYTE_ADDR(pd, field_desc(field))) ) )

#define FIELD_MASKED_BYTES(pd, field) (FIELD_BYTES(pd, field) & FIELD_MASK(pd, field))

#define BITS_MASK1(pd, field) (FIELD_MASK(pd, field) & 0xff)
#define BITS_MASK2(pd, field) (FIELD_MASK(pd, field) & (0xffffffff >> ((4 - (FIELD_BITCOUNT(pd, field) - 1) / 8) * 8)) & 0xffffff00)
#define BITS_MASK3(pd, field) (FIELD_MASK(pd, field) & (0xff << (((FIELD_BITCOUNT(pd, field) - 1) / 8) * 8)))

// Modifies a field in the packet by the given source and length [ONLY BYTE ALIGNED]
#define MODIFY_BYTEBUF_BYTEBUF(pd, dstfield, src, srclen) { \
    memcpy(FIELD_BYTE_ADDR(pd, field_desc(dstfield)), src, srclen); \
}

// Modifies a field in the packet by the given source and length (byte conversion when necessary) [MAX 4 BYTES]
// assuming `uint32_t value32' is in the scope
#define MODIFY_INT32_BYTEBUF(pd, dstfield, src, srclen) { \
    value32 = 0; \
    memcpy(&value32, src, srclen); \
    MODIFY_INT32_INT32_AUTO(pd, dstfield, value32); \
}

// Modifies a field in the packet by a uint32_t value (no byteorder conversion) [MAX 4 BYTES]
// assuming `uint32_t res32' is in the scope
#define MODIFY_INT32_INT32_BITS(pd, dstfield, value32) { \
    if(FIELD_BYTECOUNT(pd, dstfield) == 1) \
        res32 = (FIELD_BYTES(pd, dstfield) & ~FIELD_MASK(pd, dstfield)) | (value32 << (8 - FIELD_BITCOUNT(pd, dstfield)) & FIELD_MASK(pd, dstfield)); \
    else if(FIELD_BYTECOUNT(pd, dstfield) == 2) \
        res32 = (FIELD_BYTES(pd, dstfield) & ~FIELD_MASK(pd, dstfield)) | \
                (value32 &  BITS_MASK1(pd, dstfield)) | \
               ((value32 & (BITS_MASK3(pd, dstfield) >> (16 - FIELD_BITWIDTH(pd, dstfield)))) << (16 - FIELD_BITWIDTH(pd, dstfield))); \
    else \
        res32 = (FIELD_BYTES(pd, dstfield) & ~FIELD_MASK(pd, dstfield)) | \
                (value32 &  BITS_MASK1(pd, dstfield)) | \
               ((value32 & (BITS_MASK2(pd, dstfield) >> field_desc(dstfield).bitoffset)) << field_desc(dstfield).bitoffset) | \
               ((value32 & (BITS_MASK3(pd, dstfield) >> (FIELD_BYTECOUNT(pd, dstfield) * 8 - FIELD_BITWIDTH(pd, dstfield)))) << (FIELD_BYTECOUNT(pd, dstfield) * 8 - FIELD_BITWIDTH(pd, dstfield))); \
    memcpy(FIELD_BYTE_ADDR(pd, field_desc(dstfield)), &res32, FIELD_BYTECOUNT(pd, dstfield)); \
}

// Modifies a field in the packet by a uint32_t value with byte conversion (always) [MAX 4 BYTES]
// assuming `uint32_t res32' is in the scope
#define MODIFY_INT32_INT32_HTON(pd, dstfield, value32) { \
    if(FIELD_BYTECOUNT(pd, dstfield) == 1) \
        res32 = (FIELD_BYTES(pd, dstfield) & ~FIELD_MASK(pd, dstfield)) | ((value32 << (8 - FIELD_BITCOUNT(pd, dstfield))) & FIELD_MASK(pd, dstfield)); \
    else if(FIELD_BYTECOUNT(pd, dstfield) == 2) \
        res32 = (FIELD_BYTES(pd, dstfield) & ~FIELD_MASK(pd, dstfield)) | (rte_cpu_to_be_16(value32 << (16 - FIELD_BITCOUNT(pd, dstfield))) & FIELD_MASK(pd, dstfield)); \
    else \
        res32 = (FIELD_BYTES(pd, dstfield) & ~FIELD_MASK(pd, dstfield)) | (rte_cpu_to_be_32(value32 << (32 - FIELD_BITCOUNT(pd, dstfield))) & FIELD_MASK(pd, dstfield)); \
    memcpy(FIELD_BYTE_ADDR(pd, field_desc(dstfield)), &res32, FIELD_BYTECOUNT(pd, dstfield)); \
}

// Modifies a field in the packet by a uint32_t value with byte conversion when necessary [MAX 4 BYTES]
// assuming `uint32_t res32' is in the scope
#define MODIFY_INT32_INT32_AUTO(pd, dstfield, value) { \
    if(field_desc(dstfield).meta) MODIFY_INT32_INT32_BITS(pd, dstfield, value) else MODIFY_INT32_INT32_HTON(pd, dstfield, value) \
}

//TODO: This should be simplified or separated into multiple macros
// Gets the value of a field
#define GET_INT32_AUTO(pd, field) (field_desc(field).meta ? \
    (FIELD_BYTECOUNT(pd, field) == 1 ? (FIELD_MASKED_BYTES(pd, field) >> (8 - FIELD_BITCOUNT(pd, field))) : \
                                        ((FIELD_BYTES(pd, field) & BITS_MASK1(pd, field)) | \
                                        ((FIELD_BYTES(pd, field) & BITS_MASK2(pd, field)) >> field_desc(field).bitoffset) | \
                                        ((FIELD_BYTES(pd, field) & BITS_MASK3(pd, field)) >> (FIELD_BYTECOUNT(pd, field) * 8 - FIELD_BITWIDTH(pd, field))))) :\
    (rte_be_to_cpu_32(FIELD_MASKED_BYTES(pd, field)) >> (32 - FIELD_BITCOUNT(pd, field))))

// Extracts a field to the given uint32_t variable (no byteorder conversion) [MAX 4 BYTES]
#define EXTRACT_INT32_BITS(pd, field, dst) { \
    if(FIELD_BYTECOUNT(pd, field) == 1) \
        dst = FIELD_MASKED_BYTES(pd, field) >> (8 - FIELD_BITCOUNT(pd, field)); \
    else if(FIELD_BYTECOUNT(pd, field) == 2) \
        dst = (FIELD_BYTES(pd, field) & BITS_MASK1(pd, field)) | \
             ((FIELD_BYTES(pd, field) & BITS_MASK3(pd, field)) >> (16 - FIELD_BITWIDTH(pd, field))); \
    else \
        dst = (FIELD_BYTES(pd, field) & BITS_MASK1(pd, field)) | \
             ((FIELD_BYTES(pd, field) & BITS_MASK2(pd, field)) >> field_desc(field).bitoffset) | \
             ((FIELD_BYTES(pd, field) & BITS_MASK3(pd, field)) >> (FIELD_BYTECOUNT(pd, field) * 8 - FIELD_BITWIDTH(pd, field))); \
}

// Extracts a field to the given uint32_t variable with byte conversion (always) [MAX 4 BYTES]
#define EXTRACT_INT32_NTOH(pd, field, dst) { \
    if(FIELD_BYTECOUNT(pd, field) == 1) \
        dst =                  FIELD_MASKED_BYTES(pd, field)  >> (8  - FIELD_BITCOUNT(pd, field)); \
    else if(FIELD_BYTECOUNT(pd, field) == 2) \
        dst = rte_be_to_cpu_16(FIELD_MASKED_BYTES(pd, field)) >> (16 - FIELD_BITCOUNT(pd, field)); \
    else \
        dst = rte_be_to_cpu_32(FIELD_MASKED_BYTES(pd, field)) >> (32 - FIELD_BITCOUNT(pd, field)); \
}

// Extracts a field to the given uint32_t variable with byte conversion when necessary [MAX 4 BYTES]
#define EXTRACT_INT32_AUTO(pd, field, dst) { \
    if(field_desc(field).meta) EXTRACT_INT32_BITS(pd, field, dst) else EXTRACT_INT32_NTOH(pd, field, dst) \
}

// Extracts a field to the given destination [ONLY BYTE ALIGNED]
#define EXTRACT_BYTEBUF(pd, field, dst) { \
    memcpy(dst, FIELD_BYTE_ADDR(pd, field_desc(field)), FIELD_BYTEWIDTH(pd, field)); \
}

#endif // DPDK_PRIMITIVES_H

