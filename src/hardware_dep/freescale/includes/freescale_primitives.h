#ifndef FREESCALE_PRIMITIVES_H
#define FREESCALE_PRIMITIVES_H

#include <odp/api/byteorder.h>

// Returns a pointer (uint8_t*) to the byte containing the first bit of the given field in the packet
#define FIELD_BYTE_ADDR(pd, fd) (((uint8_t*)(pd)->headers[fd.header].pointer)+fd.byteoffset)

#define FIELD_BYTES(pd, field) ( \
     field_desc(field).bytecount == 1 ? (*(uint8_t*) FIELD_BYTE_ADDR(pd, field_desc(field))) : \
    (field_desc(field).bytecount == 2 ? (*(uint16_t*)FIELD_BYTE_ADDR(pd, field_desc(field))) : \
                                        (*(uint32_t*)FIELD_BYTE_ADDR(pd, field_desc(field))) ) )

#define FIELD_BITCOUNT(field) (field_desc(field).bitoffset + field_desc(field).bitwidth)

#define BITS_MASK1(field) (field_desc(field).mask & 0xff)
#define BITS_MASK2(field) (field_desc(field).mask & (0xffffffff >> ((4 - (FIELD_BITCOUNT(field) - 1) / 8) * 8)) & 0xffffff00)
#define BITS_MASK3(field) (field_desc(field).mask & (0xff << (((FIELD_BITCOUNT(field) - 1) / 8) * 8)))

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
    if(field_desc(dstfield).bytecount == 1) \
        res32 = (FIELD_BYTES(pd, dstfield) & ~field_desc(dstfield).mask) | (value32 << (8 - FIELD_BITCOUNT(dstfield)) & field_desc(dstfield).mask); \
    else if(field_desc(dstfield).bytecount == 2) \
        res32 = (FIELD_BYTES(pd, dstfield) & ~field_desc(dstfield).mask) | \
                (value32 &  BITS_MASK1(dstfield)) | \
               ((value32 & (BITS_MASK3(dstfield) >> (16 - field_desc(dstfield).bitwidth))) << (16 - field_desc(dstfield).bitwidth)); \
    else \
        res32 = (FIELD_BYTES(pd, dstfield) & ~field_desc(dstfield).mask) | \
                (value32 &  BITS_MASK1(dstfield)) | \
               ((value32 & (BITS_MASK2(dstfield) >> field_desc(dstfield).bitoffset)) << field_desc(dstfield).bitoffset) | \
               ((value32 & (BITS_MASK3(dstfield) >> (field_desc(dstfield).bytecount * 8 - field_desc(dstfield).bitwidth))) << (field_desc(dstfield).bytecount * 8 - field_desc(dstfield).bitwidth)); \
    memcpy(FIELD_BYTE_ADDR(pd, field_desc(dstfield)), &res32, field_desc(dstfield).bytecount); \
}

// Modifies a field in the packet by a uint32_t value with byte conversion (always) [MAX 4 BYTES]
// assuming `uint32_t res32' is in the scope
#define MODIFY_INT32_INT32_HTON(pd, dstfield, value32) { \
    if(field_desc(dstfield).bytecount == 1) \
        res32 = (FIELD_BYTES(pd, dstfield) & ~field_desc(dstfield).mask) | ((value32 << (8 - FIELD_BITCOUNT(dstfield))) & field_desc(dstfield).mask); \
    else if(field_desc(dstfield).bytecount == 2) \
        res32 = (FIELD_BYTES(pd, dstfield) & ~field_desc(dstfield).mask) | (odp_cpu_to_be_16(value32 << (16 - FIELD_BITCOUNT(dstfield))) & field_desc(dstfield).mask); \
    else \
        res32 = (FIELD_BYTES(pd, dstfield) & ~field_desc(dstfield).mask) | (odp_cpu_to_be_32(value32 << (32 - FIELD_BITCOUNT(dstfield))) & field_desc(dstfield).mask); \
    memcpy(FIELD_BYTE_ADDR(pd, field_desc(dstfield)), &res32, field_desc(dstfield).bytecount); \
}

// Modifies a field in the packet by a uint32_t value with byte conversion when necessary [MAX 4 BYTES]
// assuming `uint32_t res32' is in the scope
#define MODIFY_INT32_INT32_AUTO(pd, dstfield, value) { \
    if(field_desc(dstfield).meta) MODIFY_INT32_INT32_BITS(pd, dstfield, value) else MODIFY_INT32_INT32_HTON(pd, dstfield, value) \
}

//TODO: This should be simplified or separated into multiple macros
// Gets the value of a field
#define GET_INT32_AUTO(pd, field) (field_desc(field).meta ? \
    (field_desc(field).bytecount == 1 ? ((FIELD_BYTES(pd, field) & field_desc(field).mask) >> (8 - FIELD_BITCOUNT(field))) : \
                                        ((FIELD_BYTES(pd, field) & BITS_MASK1(field)) | \
                                        ((FIELD_BYTES(pd, field) & BITS_MASK2(field)) >> field_desc(field).bitoffset) | \
                                        ((FIELD_BYTES(pd, field) & BITS_MASK3(field)) >> (field_desc(field).bytecount * 8 - field_desc(field).bitwidth)))) :\
    (odp_be_to_cpu_32((FIELD_BYTES(pd, field) & field_desc(field).mask)) >> (32 - FIELD_BITCOUNT(field))))

// Extracts a field to the given uint32_t variable (no byteorder conversion) [MAX 4 BYTES]
#define EXTRACT_INT32_BITS(pd, field, dst) { \
    if(field_desc(field).bytecount == 1) \
        dst = (FIELD_BYTES(pd, field) & field_desc(field).mask) >> (8 - FIELD_BITCOUNT(field)); \
    else if(field_desc(field).bytecount == 2) \
        dst = (FIELD_BYTES(pd, field) & BITS_MASK1(field)) | \
             ((FIELD_BYTES(pd, field) & BITS_MASK3(field)) >> (16 - field_desc(field).bitwidth)); \
    else \
        dst = (FIELD_BYTES(pd, field) & BITS_MASK1(field)) | \
             ((FIELD_BYTES(pd, field) & BITS_MASK2(field)) >> field_desc(field).bitoffset) | \
             ((FIELD_BYTES(pd, field) & BITS_MASK3(field)) >> (field_desc(field).bytecount * 8 - field_desc(field).bitwidth)); \
}

// Extracts a field to the given uint32_t variable with byte conversion (always) [MAX 4 BYTES]
#define EXTRACT_INT32_NTOH(pd, field, dst) { \
    if(field_desc(field).bytecount == 1) \
        dst = (FIELD_BYTES(pd, field) & field_desc(field).mask) >> (8 - FIELD_BITCOUNT(field)); \
    else if(field_desc(field).bytecount == 2) \
        dst = odp_be_to_cpu_16((FIELD_BYTES(pd, field) & field_desc(field).mask)) >> (16 - FIELD_BITCOUNT(field)); \
    else \
        dst = odp_be_to_cpu_32((FIELD_BYTES(pd, field) & field_desc(field).mask)) >> (32 - FIELD_BITCOUNT(field)); \
}

// Extracts a field to the given uint32_t variable with byte conversion when necessary [MAX 4 BYTES]
#define EXTRACT_INT32_AUTO(pd, field, dst) { \
    if(field_desc(field).meta) EXTRACT_INT32_BITS(pd, field, dst) else EXTRACT_INT32_NTOH(pd, field, dst) \
}

// Extracts a field to the given destination [ONLY BYTE ALIGNED]
#define EXTRACT_BYTEBUF(pd, field, dst) { \
    memcpy(dst, FIELD_BYTE_ADDR(pd, field_desc(field)), field_desc(field).bytewidth); \
}

#endif

