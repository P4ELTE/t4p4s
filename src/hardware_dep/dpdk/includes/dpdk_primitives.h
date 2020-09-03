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
#pragma once

#include <rte_byteorder.h>
#include "dataplane.h"

#ifdef T4P4S_DEBUG
    #include <assert.h>
#endif

/*******************************************************************************
   Auxiliary
*******************************************************************************/

typedef struct bitfield_handle_s {
    uint8_t* byte_addr;
    int      meta; // endianness / is_host_byte_order
    int      bitwidth;
    int      bytewidth;
    int      bitcount;
    int      bytecount;
    int      bitoffset;
    int      byteoffset;
    uint32_t mask;
    int      fixed_width;
} bitfield_handle_t;

#define FIELD_FIXED_WIDTH_HDESC(f) (f != hdr_infos[fld_infos[f].header_instance].var_width_field)
#define FIELD_FIXED_POS_HDESC(f)   (f <= hdr_infos[fld_infos[f].header_instance].var_width_field || hdr_infos[fld_infos[f].header_instance].var_width_field == -1)

#define FIELD_DYNAMIC_BITWIDTH_HDESC(hdesc, f) (FIELD_FIXED_WIDTH_HDESC(f) ? fld_infos[f].bit_width : hdesc.var_width_field_bitwidth)
#define FIELD_DYNAMIC_BYTEOFFSET_HDESC(hdesc, f) (fld_infos[f].byte_offset + (FIELD_FIXED_POS_HDESC(f) ? 0 : (hdesc.var_width_field_bitwidth / 8)))

#define handle(hdesc, f) \
        ((bitfield_handle_t) \
        { \
            .byte_addr   = (((uint8_t*)hdesc.pointer)+(FIELD_DYNAMIC_BYTEOFFSET_HDESC(hdesc, f))), \
            .meta        = hdr_infos[fld_infos[f].header_instance].is_metadata, \
            .bitwidth    = FIELD_DYNAMIC_BITWIDTH_HDESC(hdesc, f), \
            .bytewidth   = (FIELD_DYNAMIC_BITWIDTH_HDESC(hdesc, f) + 7) / 8, \
            .bitcount    = FIELD_DYNAMIC_BITWIDTH_HDESC(hdesc, f) + fld_infos[f].bit_offset, /* bitwidth + bitoffset */ \
            .bytecount   = ((FIELD_DYNAMIC_BITWIDTH_HDESC(hdesc, f) + 7 + fld_infos[f].bit_offset) / 8), \
            .bitoffset   = fld_infos[f].bit_offset, \
            .byteoffset  = FIELD_DYNAMIC_BYTEOFFSET_HDESC(hdesc, f), \
            .mask        = fld_infos[f].mask, \
            .fixed_width = FIELD_FIXED_WIDTH_HDESC(f), \
        })

#define header_desc_buf(buf, w) ((header_descriptor_t) { -1, buf, -1, w })
#define header_desc_ins(pd, h)  ((pd)->headers[h])

/******************************************************************************/

#define FIELD_MASK(fd) (fd.fixed_width ? fd.mask : \
    rte_cpu_to_be_32((0xffffffff << (32 - fd.bitcount)) & (0xffffffff >> fd.bitoffset)))

#define FIELD_BYTES(fd) (  fd.bytecount == 1 ? (*(uint8_t*)  fd.byte_addr) : \
                         ( fd.bytecount == 2 ? (*(uint16_t*) fd.byte_addr) : \
                                               (*(uint32_t*) fd.byte_addr) ) )

#define FIELD_MASKED_BYTES(fd) (FIELD_BYTES(fd) & FIELD_MASK(fd))

#define BITS_MASK1(fd) (FIELD_MASK(fd) & 0xff)
#define BITS_MASK2(fd) (FIELD_MASK(fd) & (0xffffffff >> ((4 - (fd.bitcount - 1) / 8) * 8)) & 0xffffff00)
#define BITS_MASK3(fd) (FIELD_MASK(fd) & (0xff << (((fd.bitcount - 1) / 8) * 8)))

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

// Modifies a field in the packet by a uint32_t value (no byteorder conversion) [MAX 4 BYTES]
// assuming `uint32_t res32' is in the scope
#define MODIFY_INT32_INT32_BITS(dst_fd, value32) { \
    if(dst_fd.bytecount == 1) \
        res32 = (FIELD_BYTES(dst_fd) & ~FIELD_MASK(dst_fd)) | (value32 << (8 - dst_fd.bitcount) & FIELD_MASK(dst_fd)); \
    else if(dst_fd.bytecount == 2) \
        res32 = (FIELD_BYTES(dst_fd) & ~FIELD_MASK(dst_fd)) | \
                (value32 &  BITS_MASK1(dst_fd)) | \
               ((value32 & (BITS_MASK3(dst_fd) >> (16 - dst_fd.bitwidth))) << (16 - dst_fd.bitwidth)); \
    else \
        res32 = (FIELD_BYTES(dst_fd) & ~FIELD_MASK(dst_fd)) | \
                (value32 &  BITS_MASK1(dst_fd)) | \
               ((value32 & (BITS_MASK2(dst_fd) >> dst_fd.bitoffset)) << dst_fd.bitoffset) | \
               ((value32 & (BITS_MASK3(dst_fd) >> (dst_fd.bytecount * 8 - dst_fd.bitwidth))) << (dst_fd.bytecount * 8 - dst_fd.bitwidth)); \
    memcpy(dst_fd.byte_addr, &res32, dst_fd.bytecount); \
}

// Modifies a field in the packet by a uint32_t value with byte conversion (always) [MAX 4 BYTES]
// assuming `uint32_t res32' is in the scope
#define MODIFY_INT32_INT32_HTON(dst_fd, value32) { \
    if(dst_fd.bytecount == 1) \
        res32 = (FIELD_BYTES(dst_fd) & ~FIELD_MASK(dst_fd)) | ((value32 << (8 - dst_fd.bitcount)) & FIELD_MASK(dst_fd)); \
    else if(dst_fd.bytecount == 2) \
        res32 = (FIELD_BYTES(dst_fd) & ~FIELD_MASK(dst_fd)) | (rte_cpu_to_be_16(value32 << (16 - dst_fd.bitcount)) & FIELD_MASK(dst_fd)); \
    else \
        res32 = (FIELD_BYTES(dst_fd) & ~FIELD_MASK(dst_fd)) | (rte_cpu_to_be_32(value32 << (32 - dst_fd.bitcount)) & FIELD_MASK(dst_fd)); \
    memcpy(dst_fd.byte_addr, &res32, dst_fd.bytecount); \
}

// Modifies a field in the packet by a uint32_t value with byte conversion when necessary [MAX 4 BYTES]
// assuming `uint32_t res32' is in the scope
#define MODIFY_INT32_INT32_AUTO(dst_fd, value32) { \
    if(dst_fd.meta) { MODIFY_INT32_INT32_BITS(dst_fd, value32) } else { MODIFY_INT32_INT32_HTON(dst_fd, value32) } \
}

/*******************************************************************************
   Extract - expression (unpack value and return it)
*******************************************************************************/

//TODO: This should be simplified or separated into multiple macros
// Gets the value of a field
#define GET_INT32_AUTO(fd) (fd.meta ? \
    (fd.bytecount == 1 ? (FIELD_MASKED_BYTES(fd) >> (8 - fd.bitcount)) : \
                                        ((FIELD_BYTES(fd) & BITS_MASK1(fd)) | \
                                        ((FIELD_BYTES(fd) & BITS_MASK2(fd)) >> fd.bitoffset) | \
                                        ((FIELD_BYTES(fd) & BITS_MASK3(fd)) >> (fd.bytecount * 8 - fd.bitwidth)))) :\
    (fd.bytecount == 1 ? (FIELD_MASKED_BYTES(fd) >> (8 - fd.bitcount)) : \
        (fd.bytecount == 2 ? (rte_be_to_cpu_16(FIELD_MASKED_BYTES(fd)) >> (16 - fd.bitcount)) : \
            (rte_be_to_cpu_32(FIELD_MASKED_BYTES(fd)) >> (32 - fd.bitcount)))))

/*******************************************************************************
   Extract - statement (unpack value to a destination variable)
*******************************************************************************/

// Extracts a field to the given uint32_t variable with byte conversion (always) [MAX 4 BYTES]
#define EXTRACT_INT32_NTOH(fd, dst) { \
    if(fd.bytecount == 1) \
        dst =                  FIELD_MASKED_BYTES(fd) >> (8  - fd.bitcount); \
    else if(fd.bytecount == 2) \
        dst = rte_be_to_cpu_16(FIELD_MASKED_BYTES(fd)) >> (16 - fd.bitcount); \
    else \
        dst = rte_be_to_cpu_32(FIELD_MASKED_BYTES(fd)) >> (32 - fd.bitcount); \
}

// Extracts a field to the given uint32_t variable (no byteorder conversion) [MAX 4 BYTES]
#define EXTRACT_INT32_BITS(fd, dst) { \
    if(fd.bytecount == 1) \
        dst = FIELD_MASKED_BYTES(fd) >> (8 - fd.bitcount); \
    else if(fd.bytecount == 2) \
        dst = (FIELD_BYTES(fd) & BITS_MASK1(fd)) | \
             ((FIELD_BYTES(fd) & BITS_MASK3(fd)) >> (16 - fd.bitwidth)); \
    else \
        dst = (FIELD_BYTES(fd) & BITS_MASK1(fd)) | \
             ((FIELD_BYTES(fd) & BITS_MASK2(fd)) >> fd.bitoffset) | \
             ((FIELD_BYTES(fd) & BITS_MASK3(fd)) >> (fd.bytecount * 8 - fd.bitwidth)); \
}

// Extracts a field to the given uint32_t variable with byte conversion when necessary [MAX 4 BYTES]
#define EXTRACT_INT32_AUTO(fd, dst) { \
    if(fd.meta) { EXTRACT_INT32_BITS(fd, dst) } else { EXTRACT_INT32_NTOH(fd, dst) } \
}

// Extracts a field to the given destination [ONLY BYTE ALIGNED]
#define EXTRACT_BYTEBUF(fd, dst) { \
    memcpy(dst, fd.byte_addr, fd.bytewidth); \
}


/*******************************************************************************
   Interface
*******************************************************************************/

// extract

#define GET_INT32_AUTO_PACKET(pd , h, f) GET_INT32_AUTO(handle(header_desc_ins(pd , h), f))
#define GET_INT32_AUTO_BUFFER(buf, w, f) GET_INT32_AUTO(handle(header_desc_buf(buf, w), f))

#define EXTRACT_BYTEBUF_PACKET(pd , h, f, dst) EXTRACT_BYTEBUF(handle(header_desc_ins(pd , h), f), dst)
#define EXTRACT_BYTEBUF_BUFFER(buf, w, f, dst) EXTRACT_BYTEBUF(handle(header_desc_buf(buf, w), f), dst)

#define EXTRACT_INT32_AUTO_PACKET(pd , h, f, dst) EXTRACT_INT32_AUTO(handle(header_desc_ins(pd , h), f), dst)
#define EXTRACT_INT32_AUTO_BUFFER(buf, w, f, dst) EXTRACT_INT32_AUTO(handle(header_desc_buf(buf, w), f), dst)

#define EXTRACT_INT32_BITS_PACKET(pd , h, f, dst) EXTRACT_INT32_BITS(handle(header_desc_ins(pd , h), f), dst)
#define EXTRACT_INT32_BITS_BUFFER(buf, w, f, dst) EXTRACT_INT32_BITS(handle(header_desc_buf(buf, w), f), dst)

// modify

#define MODIFY_BYTEBUF_BYTEBUF_PACKET(pd , h, f, src, srclen) MODIFY_BYTEBUF_BYTEBUF(handle(header_desc_ins(pd , h), f), src, srclen);
#define MODIFY_BYTEBUF_BYTEBUF_BUFFER(buf, w, f, src, srclen) MODIFY_BYTEBUF_BYTEBUF(handle(header_desc_buf(buf, w), f), src, srclen);

#define MODIFY_INT32_BYTEBUF_PACKET(pd , h, f, src, srclen) MODIFY_INT32_BYTEBUF(handle(header_desc_ins(pd , h), f), src, srclen);
#define MODIFY_INT32_BYTEBUF_BUFFER(buf, w, f, src, srclen) MODIFY_INT32_BYTEBUF(handle(header_desc_buf(buf, w), f), src, srclen);

#define MODIFY_INT32_INT32_BITS_PACKET(pd , h, f, value32) MODIFY_INT32_INT32_BITS(handle(header_desc_ins(pd , h), f), value32);
#define MODIFY_INT32_INT32_BITS_BUFFER(buf, w, f, value32) MODIFY_INT32_INT32_BITS(handle(header_desc_buf(buf, w), f), value32);

#define MODIFY_INT32_INT32_AUTO_BUFFER(buf, w, f, value32) MODIFY_INT32_INT32_AUTO(handle(header_desc_buf(buf, w), f), value32);



// #define MODIFY_INT32_INT32_AUTO_PACKET(pd , h, f, value32) MODIFY_INT32_INT32_AUTO(handle(header_desc_ins(pd , h), f), value32);

// TODO simplify all other interface macros, too
// #define MODIFY_INT32_INT32_AUTO_PACKET(pd , h, f, value32) MODIFY_INT32_INT32_AUTO(handle(header_desc_ins(pd , h), f), value32);
static int MODIFY_INT32_INT32_AUTO_PACKET(packet_descriptor_t* pd, enum header_instance_e h, enum field_instance_e f, uint32_t value32) {
    int res32;
    MODIFY_INT32_INT32_AUTO(handle(header_desc_ins(pd , h), f), value32);
    return res32;
}

typedef struct target_fld_s {
    packet_descriptor_t*   pd;
    enum header_instance_e hdr;
    enum field_instance_e  fld;
} fldT;

typedef struct target_buf_s {
    uint8_t* buf;
    int      w;
    enum field_instance_e fld;
} bufT;

static int set_field(fldT f[], bufT b[], uint32_t value32, int bit_width) {
#ifdef T4P4S_DEBUG
    // exactly one of `f` and `b` have to be non-zero
    assert((f == 0) != (b == 0));
#endif

    if (f != 0) {
        fldT fld = f[0];
        debug("    " T4LIT(=,field) " Modifying field " T4LIT(%s,header) "." T4LIT(%s,field) "/" T4LIT(%d) "b (" T4LIT(%d) "B) = " T4LIT(%d) " (0x" T4LIT(%0*x) ")\n",
              header_instance_names[fld.hdr],
              field_names[fld.fld],
              bit_width,
              bit_width <= 8 ? 1 : bit_width <= 16 ? 2 : 4,
              bit_width <= 8 ? (uint8_t)value32 : bit_width <= 16 ? (uint16_t)value32 : value32,
              (bit_width+7)/8*2,
              bit_width <= 8 ? (uint8_t)value32 : bit_width <= 16 ? (uint16_t)value32 : value32);

        int res32;
        MODIFY_INT32_INT32_AUTO(handle(header_desc_ins(fld.pd, fld.hdr), fld.fld), value32);
        return res32;
    }

    // TODO implement this case, too
    if (b != 0)   rte_exit(2, "TODO unimplemented portion of set_field");

    // TODO should never happen; exit
    return -1;
}

void transfer_to_egress(packet_descriptor_t* pd);

/*******************************************************************************
   Extract - statement
*******************************************************************************/

int extract_egress_port(packet_descriptor_t* pd);
int extract_ingress_port(packet_descriptor_t* pd);
