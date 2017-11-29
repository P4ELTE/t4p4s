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

#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include "dataplane.h"

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#include "primitives_le.h"
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#include "primitives_be.h"
#else
#error "Unsupported byte order."
#endif

#include <endian.h>
#include <stdint.h>
#include <string.h>

#define FIELD_BITCOUNT(pd, field) (field_desc(pd, field).bitwidth + field_desc(pd, field).bitoffset)

#define FIELD_MASK(pd, field) (field_desc(pd, field).fixed_width ? field_desc(pd, field).mask : \
    htobe32((0xffffffff << (32 - FIELD_BITCOUNT(pd, field))) & (0xffffffff >> field_desc(pd, field).bitoffset)))

#define FIELD_BYTES(pd, field) ( \
     field_desc(pd, field).bytecount == 1 ? (*(uint8_t*)  field_desc(pd, field).byte_addr) : \
    (field_desc(pd, field).bytecount == 2 ? (*(uint16_t*) field_desc(pd, field).byte_addr) : \
                                            (*(uint32_t*) field_desc(pd, field).byte_addr) ) )

#define FIELD_MASKED_BYTES(pd, field) (FIELD_BYTES(pd, field) & FIELD_MASK(pd, field))

// Modifies a field in the packet by the given source and length [ONLY BYTE ALIGNED]
#define MODIFY_BYTEBUF_BYTEBUF(pd, dstfield, src, srclen) { \
    /*TODO: If the src contains a signed negative value, than the following memset is incorrect*/ \
    memset(field_desc(pd, dstfield).byte_addr, 0, field_desc(pd, dstfield).bytewidth - srclen); \
    memcpy(field_desc(pd, dstfield).byte_addr + (field_desc(pd, dstfield).bytewidth - srclen), src, srclen); \
}

// Modifies a field in the packet by the given source and length (byte conversion when necessary) [MAX 4 BYTES]
// assuming `uint32_t value32' is in the scope
#define MODIFY_INT32_BYTEBUF(pd, dstfield, src, srclen) { \
    value32 = 0; \
    memcpy(&value32, src, srclen); \
    MODIFY_INT32_INT32_AUTO(pd, dstfield, value32); \
}

// Extracts a field to the given destination [ONLY BYTE ALIGNED]
#define EXTRACT_BYTEBUF(pd, field, dst) { \
    memcpy(dst, field_desc(pd, field).byte_addr, field_desc(pd, field).bytewidth); \
}

#endif

