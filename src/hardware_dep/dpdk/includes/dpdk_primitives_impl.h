// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_lib.h"


bitfield_handle_t dst(header_instance_e hdesc, field_instance_e fld);

//header_instance_e to_hdr(field_instance_e fld);
//static inline header_instance_e to_hdr(field_instance_e fld) {
//    return fld_infos[fld].header_instance;
//}

// Extract operations
/*
static inline uint32_t FLD_MASK(bitfield_handle_t fd) {
    if (fd.fixed_width)  return fd.mask;

    uint32_t mask_offset = ~0 >> fd.bitoffset;
    uint32_t mask_len = ~0 << (32 - fd.bitcount);
    return mask_offset & mask_len;
}

static inline uint32_t FLD_BYTES(bitfield_handle_t fd) {
    return fd.bytecount == 1 ? (*(uint8_t*)  fd.pointer) :
           fd.bytecount == 2 ? (*(uint16_t*) fd.pointer) :
                               (*(uint32_t*) fd.pointer);
}

uint32_t GET32_META_IMPL(bitfield_handle_t fd);

static inline uint32_t GET32_FLD_IMPL(bitfield_handle_t src) {
    if (src.is_t4p4s_byte_order)   return FLD_BYTES(src);

    uint32_t masked = net2t4p4s_4(FLD_BYTES(src)) & FLD_MASK(src);
    int downshift = 32 - src.bitoffset - src.bitwidth;
    return masked >> downshift;
}

// Extracts a field to the given destination [ONLY BYTE ALIGNED]
static inline void GET_BUF_IMPL(void* dst, bitfield_handle_t src) {
    memcpy(dst, src.pointer, src.bytewidth);
}
*/

// Modify operations

void SET32_IMPL(bitfield_handle_t dst_fd, uint32_t value32);

void SET_BUF_IMPL(bitfield_handle_t dst, void* src, int srclen);
void SET32_BUF_IMPL(bitfield_handle_t dst, void* src, int srclen);
void SET32_BITS_IMPL(bitfield_handle_t dst, uint32_t value32);

/*
static inline void MODIFY_BUF_IMPL(bitfield_handle_t dst, void* src, int src_bytewidth) {
    memset(dst.pointer, 0, dst.bytewidth - src_bytewidth);
    memcpy(dst.pointer + (dst.bytewidth - src_bytewidth), src, src_bytewidth);
}

static inline uint32_t bitshift(int container_length, bitfield_handle_t fd) {
    return container_length - fd.bitoffset - fd.bitwidth;
}

static inline uint32_t MASK_AT(int shift, uint32_t value32, uint32_t mask) {
    return ((value32 << shift) & mask) >> shift;
}

// Modifies a field in the packet by a uint32_t value with byte conversion (always) [MAX 4 BYTES]
static inline void MODIFY32_T4P4S2NET_IMPL(bitfield_handle_t dst, uint32_t value32) {
    uint32_t res32 = FLD_BYTES(dst) & ~FLD_MASK(dst);
    res32 |= t4p4s2net(dst, value32 << (padded_bytecount(dst) - dst.bitcount)) & FLD_MASK(dst);
    memcpy(dst.pointer, &res32, dst.bytecount);
}

static inline void MODIFY32_T4P4S_ORDER(bitfield_handle_t dst, uint32_t value32) {
    if      (dst.bytecount == 1)   *(uint8_t*)dst.pointer = (uint8_t)value32;
    else if (dst.bytecount == 1)  *(uint16_t*)dst.pointer = (uint16_t)value32;
    else                          *(uint32_t*)dst.pointer = value32;
}

static inline void MODIFY32_IMPL_NET_ORDER(bitfield_handle_t dst, uint32_t value32) {
    int upshift = 32 - dst.bitoffset - dst.bitwidth;

    uint32_t old_content = t4p4s2net_4(*(uint32_t*)(dst.pointer)) & ~FLD_MASK(dst);
    uint32_t new_content = (value32 << upshift) & FLD_MASK(dst);

    *(uint32_t*)(dst.pointer) = net2t4p4s_4(old_content | new_content);
}

// Modifies a field in the packet by a uint32_t value with byte conversion when necessary [MAX 4 BYTES]
static inline void MODIFY32_IMPL(bitfield_handle_t dst, uint32_t value32) {
    dst.is_t4p4s_byte_order ? MODIFY32_T4P4S_ORDER(dst, value32) : MODIFY32_IMPL_NET_ORDER(dst, value32);
}

// Modifies a field in the packet by the given source and length (byte conversion when necessary) [MAX 4 BYTES]
static inline void MODIFY32_BUF_IMPL(bitfield_handle_t dst, void* src, int src_bytewidth) {
    uint32_t value32 = 0;
    memcpy(&value32, src, src_bytewidth);
    MODIFY32_IMPL(dst, value32);
}
*/

