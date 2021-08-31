// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Eotvos Lorand University, Budapest, Hungary

// this file is directly included from dpdk_primitives.h, no need for a "#pragma once"

#include <rte_byteorder.h>
#include "dataplane.h"
#include "util_packet.h"

#ifdef T4P4S_DEBUG
    #include <assert.h>
#endif

/*******************************************************************************
   Auxiliary
*******************************************************************************/

#define FLDINFOS(fld) (hdr_infos[fld_infos[fld].header_instance])

#define FLD_IS_FIXED_WIDTH(fld) (fld != FLDINFOS(fld).var_width_field)
#define FLD_IS_FIXED_POS(fld)   (FLDINFOS(fld).var_width_field == -1 || fld <= FLDINFOS(fld).var_width_field)

#define FLD_BITWIDTH(hdesc, fld) (FLD_IS_FIXED_WIDTH(fld) ? fld_infos[fld].bit_width : hdesc.var_width_field_bitwidth)
#define FLD_BYTEOFFSET(hdesc, fld) (fld_infos[fld].byte_offset + (FLD_IS_FIXED_POS(fld) ? 0 : (hdesc.var_width_field_bitwidth / 8)))

#define handle(hdesc, fld) \
        ((bitfield_handle_t) \
        { \
            .byte_addr   = (((uint8_t*)hdesc.pointer)+(FLD_BYTEOFFSET(hdesc, fld))), \
            .meta        = FLDINFOS(fld).is_metadata, \
            .bitwidth    = FLD_BITWIDTH(hdesc, fld), \
            .bytewidth   = (FLD_BITWIDTH(hdesc, fld) + 7) / 8, \
            .bitcount    = FLD_BITWIDTH(hdesc, fld) + fld_infos[fld].bit_offset, /* bitwidth + bitoffset */ \
            .bytecount   = ((FLD_BITWIDTH(hdesc, fld) + 7 + fld_infos[fld].bit_offset) / 8), \
            .bitoffset   = fld_infos[fld].bit_offset, \
            .byteoffset  = FLD_BYTEOFFSET(hdesc, fld), \
            .mask        = fld_infos[fld].mask, \
            .fixed_width = FLD_IS_FIXED_WIDTH(fld), \
        })

#define header_desc_buf(buf, w) ((header_descriptor_t) { -1, buf, -1, w })
#define header_desc_ins(pd, h)  ((pd)->headers[h])

/******************************************************************************/

#define FLD_MASK(fd) (fd.fixed_width ? fd.mask : \
    rte_cpu_to_be_32((~0U << (32 - fd.bitcount)) & (~0U >> fd.bitoffset)))

#define FLD_BYTES(fd) (  fd.bytecount == 1 ? (*(uint8_t*)  fd.byte_addr) : \
                         ( fd.bytecount == 2 ? (*(uint16_t*) fd.byte_addr) : \
                                               (*(uint32_t*) fd.byte_addr) ) )

#define FLD_MASKED_BYTES(fd) (FLD_BYTES(fd) & FLD_MASK(fd))

#define BYTECOUNT(fd)  ((fd.bitcount - 1) / 8)

#define MASK_LOW(fd) (FLD_MASK(fd) & 0xff)
#define MASK_MID(fd) (FLD_MASK(fd) & (~0U >> ((4 - BYTECOUNT(fd)) * 8)) & ~0xff)
#define MASK_TOP(fd) (FLD_MASK(fd) & (0xff << (BYTECOUNT(fd) * 8)))

/*******************************************************************************
   Modify - statement - bytebuf
*******************************************************************************/

// Modifies a field in the packet by the given source and length [ONLY BYTE ALIGNED]
#define MODIFY_BYTEBUF_BYTEBUF(dst_fd, src, srclen) { \
    /*TODO: If the src contains a signed negative value, than the following memset is incorrect*/ \
    memset(dst_fd.byte_addr, 0, dst_fd.bytewidth - srclen); \
    memcpy(dst_fd.byte_addr + (dst_fd.bytewidth - srclen), src, srclen); \
}

/*******************************************************************************
   Modify - statement - int32
*******************************************************************************/

// Modifies a field in the packet by the given source and length (byte conversion when necessary) [MAX 4 BYTES]
// assuming `uint32_t value32' is in the scope
#define MODIFY_INT32_BYTEBUF(dst_fd, src, srclen) { \
    value32 = 0; \
    memcpy(&value32, src, srclen); \
    MODIFY_INT32_INT32_AUTO(dst_fd, value32); \
}

#define MASK_AT(value32,mask,bitcount) ((value32 & ((mask) >> (bitcount))) << (bitcount))

// Modifies a field in the packet by a uint32_t value (no byteorder conversion) [MAX 4 BYTES]
// assuming `uint32_t res32' is in the scope
#define MODIFY_INT32_INT32_BITS(dst_fd, value32) { \
    { \
        uint32_t res32 = (FLD_BYTES(dst_fd) & ~FLD_MASK(dst_fd)); \
        if (dst_fd.bytecount == 1) { \
            res32 |= (value32 << (8 - dst_fd.bitcount) & FLD_MASK(dst_fd)); \
        } else if (dst_fd.bytecount == 2) { \
            res32 |= MASK_AT(value32, MASK_LOW(dst_fd), 0); \
            res32 |= MASK_AT(value32, MASK_TOP(dst_fd), 16 - dst_fd.bitwidth); \
        } else { \
            res32 |= MASK_AT(value32, MASK_LOW(dst_fd), 0); \
            res32 |= MASK_AT(value32, MASK_MID(dst_fd), dst_fd.bitoffset); \
            res32 |= MASK_AT(value32, MASK_TOP(dst_fd), dst_fd.bytecount * 8 - dst_fd.bitwidth); \
        } \
        memcpy(dst_fd.byte_addr, &res32, dst_fd.bytecount); \
    } \
}

// Modifies a field in the packet by a uint32_t value with byte conversion (always) [MAX 4 BYTES]
// assuming `uint32_t res32' is in the scope
#define MODIFY_INT32_INT32_HTON(dst_fd, value32) { \
    { \
        uint32_t res32 = (FLD_BYTES(dst_fd) & ~FLD_MASK(dst_fd)); \
        if (dst_fd.bytecount == 1) \
            res32 |= (value32 << (8 - dst_fd.bitcount)) & FLD_MASK(dst_fd); \
        else if (dst_fd.bytecount == 2) \
            res32 |= rte_cpu_to_be_16(value32 << (16 - dst_fd.bitcount)) & FLD_MASK(dst_fd); \
        else \
            res32 |= rte_cpu_to_be_32(value32 << (32 - dst_fd.bitcount)) & FLD_MASK(dst_fd); \
        memcpy(dst_fd.byte_addr, &res32, dst_fd.bytecount); \
    } \
}

// Modifies a field in the packet by a uint32_t value with byte conversion when necessary [MAX 4 BYTES]
// assuming `uint32_t res32' is in the scope
#define MODIFY_INT32_INT32_AUTO(dst_fd, value32) { \
    if (dst_fd.meta) { MODIFY_INT32_INT32_BITS(dst_fd, value32) } else { MODIFY_INT32_INT32_HTON(dst_fd, value32) } \
}

/*******************************************************************************
   Extract - expression (unpack value and return it)
*******************************************************************************/

//TODO: This should be simplified or separated into multiple macros
// Gets the value of a field
#define GET_INT32_AUTO(fd) (fd.meta ? \
    (fd.bytecount == 1 ? (FLD_MASKED_BYTES(fd) >> (8 - fd.bitcount)) : \
                                        ((FLD_BYTES(fd) & MASK_LOW(fd)) | \
                                        ((FLD_BYTES(fd) & MASK_MID(fd)) >> fd.bitoffset) | \
                                        ((FLD_BYTES(fd) & MASK_TOP(fd)) >> (fd.bytecount * 8 - fd.bitwidth)))) :\
    (fd.bytecount == 1 ? (FLD_MASKED_BYTES(fd) >> (8 - fd.bitcount)) : \
        (fd.bytecount == 2 ? (rte_be_to_cpu_16(FLD_MASKED_BYTES(fd)) >> (16 - fd.bitcount)) : \
            (rte_be_to_cpu_32(FLD_MASKED_BYTES(fd)) >> (32 - fd.bitcount)))))

/*******************************************************************************
   Extract - statement (unpack value to a destination variable)
*******************************************************************************/

// Extracts a field to the given uint32_t variable with byte conversion (always) [MAX 4 BYTES]
#define EXTRACT_INT32_NTOH(fd, dst) { \
    if (fd.bytecount == 1) \
        dst =                  FLD_MASKED_BYTES(fd) >> (8  - fd.bitcount); \
    else if (fd.bytecount == 2) \
        dst = rte_be_to_cpu_16(FLD_MASKED_BYTES(fd)) >> (16 - fd.bitcount); \
    else \
        dst = rte_be_to_cpu_32(FLD_MASKED_BYTES(fd)) >> (32 - fd.bitcount); \
}

// Extracts a field to the given uint32_t variable (no byteorder conversion) [MAX 4 BYTES]
#define EXTRACT_INT32_BITS(fd, dst) { \
    if (fd.bytecount == 1) \
        dst = FLD_MASKED_BYTES(fd) >> (8 - fd.bitcount); \
    else if (fd.bytecount == 2) \
        dst = (FLD_BYTES(fd) & MASK_LOW(fd)) | \
             ((FLD_BYTES(fd) & MASK_TOP(fd)) >> (16 - fd.bitwidth)); \
    else \
        dst = (FLD_BYTES(fd) & MASK_LOW(fd)) | \
             ((FLD_BYTES(fd) & MASK_MID(fd)) >> fd.bitoffset) | \
             ((FLD_BYTES(fd) & MASK_TOP(fd)) >> (fd.bytecount * 8 - fd.bitwidth)); \
}

// Extracts a field to the given uint32_t variable with byte conversion when necessary [MAX 4 BYTES]
#define EXTRACT_INT32_AUTO(fd, dst) { \
    if (fd.meta) { EXTRACT_INT32_BITS(fd, dst) } else { EXTRACT_INT32_NTOH(fd, dst) } \
}

// Extracts a field to the given destination [ONLY BYTE ALIGNED]
#define EXTRACT_BYTEBUF(fd, dst) { \
    memcpy(dst, fd.byte_addr, fd.bytewidth); \
}


/*******************************************************************************/

void set_field(fldT f[], bufT b[], uint32_t value32, int bit_width);

void MODIFY_INT32_INT32_AUTO_PACKET(packet_descriptor_t* pd, header_instance_t h, field_instance_t f, uint32_t value32);
