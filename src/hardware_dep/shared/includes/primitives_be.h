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

#ifndef PRIMITIVES_BE_H
#define PRIMITIVES_BE_H

#ifndef PRIMITIVES_H
#error "Do not include \"primitives_be.h\" directly, use \"primitives.h\" instead."
#endif

// Modifies a field in the packet by a uint32_t value (no byteorder conversion) [MAX 4 BYTES]
// assuming `uint32_t res32' is in the scope
#define MODIFY_INT32_INT32_BITS(pd, dstfield, value) { \
    res32 = ((FIELD_BYTES(pd, dstfield) & ~FIELD_MASK(pd, dstfield)) | (value << (field_desc(pd, dstfield).bytecount * 8 - field_desc(pd, dstfield).bitwidth - field_desc(pd, dstfield).bitoffset))) << (32 - field_desc(pd, dstfield).bytecount * 8); \
    memcpy(field_desc(pd, dstfield).byte_addr, &res32, field_desc(pd, dstfield).bytecount); \
}

// Modifies a field in the packet by a uint32_t value with byte conversion (always) [MAX 4 BYTES]
// assuming `uint32_t res32' is in the scope
#define MODIFY_INT32_INT32_HTON(pd, dstfield, value) MODIFY_INT32_INT32_BITS(pd, dstfield, value)

// Modifies a field in the packet by a uint32_t value with byte conversion when necessary [MAX 4 BYTES]
// assuming `uint32_t res32' is in the scope
#define MODIFY_INT32_INT32_AUTO(pd, dstfield, value) MODIFY_INT32_INT32_BITS(pd, dstfield, value)

// Gets the value of a field
#define GET_INT32_AUTO(pd, field) (FIELD_MASKED_BYTES(pd, field)) >> (field_desc(pd, field).bytecount * 8 - FIELD_BITCOUNT(pd, field))

// Extracts a field to the given uint32_t variable (no byteorder conversion) [MAX 4 BYTES]
#define EXTRACT_INT32_BITS(pd, field, dst) { \
    dst = GET_INT32_AUTO(pd, field); \
}

// Extracts a field to the given uint32_t variable with byte conversion (always) [MAX 4 BYTES]
#define EXTRACT_INT32_NTOH(pd, field, dst) EXTRACT_INT32_BITS(pd, field, dst)

// Extracts a field to the given uint32_t variable with byte conversion when necessary [MAX 4 BYTES]
#define EXTRACT_INT32_AUTO(pd, field, dst) EXTRACT_INT32_BITS(pd, field, dst)

#endif
