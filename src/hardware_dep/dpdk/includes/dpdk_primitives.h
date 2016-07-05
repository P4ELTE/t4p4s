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
#define FIELD_BYTE_ADDR(pd, fd) (((uint8_t*)(pd)->headers[fd.header].pointer)+fd.byteoffset)

// Converts a big endian field value to a little endian value
#define NTOH_FIELD(val, fd) ( \
    rte_be_to_cpu_32((((val << (32 - fd.bitoffset - fd.bitwidth)) & (0x000000ff << (4 - fd.bytewidth) * 8)) >> (fd.bytewidth * 8 - fd.bitwidth)) | \
                      ((val << (32 - fd.bitoffset - fd.bitwidth)) & (0xffffff00 << (4 - fd.bytewidth) * 8)) ) )

// Converts a little endian value to a big endian field value
#define HTON_FIELD(val, fd) ( \
    rte_cpu_to_be_32(((val & (0xff000000 >> (4 - fd.bytewidth) * 8)) << (fd.bytewidth * 8 - fd.bitwidth)) | \
                      (val & (0x00ffffff >> (4 - fd.bytewidth) * 8))) \
        >> (32 - fd.bitoffset - fd.bitwidth) )

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
    res32 = (*(uint32_t*)FIELD_BYTE_ADDR(pd, field_desc(dstfield)) & ~field_desc(dstfield).mask) | (value32 & (field_desc(dstfield).mask >> field_desc(dstfield).bitoffset)) << field_desc(dstfield).bitoffset; \
    memcpy(FIELD_BYTE_ADDR(pd, field_desc(dstfield)), &res32, field_desc(dstfield).bytecount); /* the compiler optimises this and cuts off the shifting/masking stuff whenever the bitoffset is 0 */ \
}

// Modifies a field in the packet by a uint32_t value with byte conversion (always) [MAX 4 BYTES]
// assuming `uint32_t res32' is in the scope
#define MODIFY_INT32_INT32_HTON(pd, dstfield, value32) { \
    if(field_desc(dstfield).bytewidth == 1) \
        res32 = (*(uint16_t*)FIELD_BYTE_ADDR(pd, field_desc(dstfield)) & ~field_desc(dstfield).mask) | (value32 & (field_desc(dstfield).mask >> field_desc(dstfield).bitoffset)) << field_desc(dstfield).bitoffset; \
    else \
        res32 = (*(uint32_t*)FIELD_BYTE_ADDR(pd, field_desc(dstfield)) & ~field_desc(dstfield).mask) | HTON_FIELD(value32 & (field_desc(dstfield).mask >> field_desc(dstfield).bitoffset), field_desc(dstfield)); \
    memcpy(FIELD_BYTE_ADDR(pd, field_desc(dstfield)), &res32, field_desc(dstfield).bytecount); \
}

// Modifies a field in the packet by a uint32_t value with byte conversion when necessary [MAX 4 BYTES]
// assuming `uint32_t res32' is in the scope
#define MODIFY_INT32_INT32_AUTO(pd, dstfield, value) { \
    if(field_desc(dstfield).meta) MODIFY_INT32_INT32_BITS(pd, dstfield, value) else MODIFY_INT32_INT32_HTON(pd, dstfield, value) \
}

// Gets the value of a field
#define GET_INT32_AUTO(pd, field) ( \
    (field_desc(field).meta || field_desc(field).bytewidth == 1) ? ((uint32_t)((*(uint32_t*)FIELD_BYTE_ADDR(pd, field_desc(field)) & (uint32_t)field_desc(field).mask) >> field_desc(field).bitoffset)) : \
    (NTOH_FIELD((*(uint32_t*)FIELD_BYTE_ADDR(pd, field_desc(field)) & (uint32_t)field_desc(field).mask), field_desc(field))))

// Extracts a field to the given uint32_t variable (no byteorder conversion) [MAX 4 BYTES]
#define EXTRACT_INT32_BITS(pd, field, dst) { \
    if(field_desc(field).bytecount == 1) \
        dst = (uint32_t)((*(uint8_t*) FIELD_BYTE_ADDR(pd, field_desc(field)) & (uint8_t) field_desc(field).mask) >> field_desc(field).bitoffset); \
    else if(field_desc(field).bytecount == 2) \
        dst = (uint32_t)((*(uint16_t*)FIELD_BYTE_ADDR(pd, field_desc(field)) & (uint16_t)field_desc(field).mask) >> field_desc(field).bitoffset); \
    else \
        dst = (uint32_t)((*(uint32_t*)FIELD_BYTE_ADDR(pd, field_desc(field)) & (uint32_t)field_desc(field).mask) >> field_desc(field).bitoffset); \
}

// Extracts a field to the given uint32_t variable with byte conversion (always) [MAX 4 BYTES]
#define EXTRACT_INT32_NTOH(pd, field, dst) { \
    if(field_desc(field).bytecount == 1)      /*bytecount=1, bytewidth=1*/\
        dst = (uint32_t)((*(uint8_t*) FIELD_BYTE_ADDR(pd, field_desc(field)) & (uint8_t) field_desc(field).mask) >> field_desc(field).bitoffset); \
    else if(field_desc(field).bytewidth == 1) /*bytecount=2, bytewidth=1*/ \
        dst = (uint32_t)((*(uint16_t*)FIELD_BYTE_ADDR(pd, field_desc(field)) & (uint16_t)field_desc(field).mask) >> field_desc(field).bitoffset); \
    else if(field_desc(field).bytecount == 2) /*bytecount=2, bytewidth=2*/ \
        dst = NTOH_FIELD((uint32_t)(*(uint16_t*)FIELD_BYTE_ADDR(pd, field_desc(field)) & (uint16_t)field_desc(field).mask), field_desc(field)); \
    else                                      /*bytecount>2, bytewidth>=2*/ \
        dst = NTOH_FIELD(          (*(uint32_t*)FIELD_BYTE_ADDR(pd, field_desc(field)) & (uint32_t)field_desc(field).mask), field_desc(field)); \
}

// Extracts a field to the given uint32_t variable with byte conversion when necessary [MAX 4 BYTES]
#define EXTRACT_INT32_AUTO(pd, field, dst) { \
    if(field_desc(field).meta) EXTRACT_INT32_BITS(pd, field, dst) else EXTRACT_INT32_NTOH(pd, field, dst) \
}

// Extracts a field to the given destination [ONLY BYTE ALIGNED]
#define EXTRACT_BYTEBUF(pd, field, dst) { \
    memcpy(dst, FIELD_BYTE_ADDR(pd, field_desc(field)), field_desc(field).bytewidth); \
}

#endif // DPDK_PRIMITIVES_H

