// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#pragma once
#include "gen_defs.h"
#include "dpdk_lib_byteorder.h"


//header_instance_e to_hdr(field_instance_e fld);
INLINING header_instance_e to_hdr(field_instance_e fld) {
    return fld_infos[fld].header_instance;
}


// Function based set field

typedef enum {
    ENDIAN_CONVERT_ALWAYS,
    ENDIAN_CONVERT_AS_NEEDED,
    ENDIAN_KEEP,
    ENDIAN_NET, // the source is an array of bytes, not an int
} endian_strategy_t;

typedef enum {
    SRCDST_BUF,
    SRCDST_PKT,
    SRCDST_HANDLE,
    SRCDST_32,
} srcdst_type_t;

typedef struct {
    srcdst_type_t srcdst_type;

    union {
        packet_descriptor_t* pd;
        struct {
            void* buf;
            int width;
        };
        bitfield_handle_t fd;
        uint32_t value32;
    };
} srcdst_t;

INLINING srcdst_t dst_buf(void* buf, int width) {
    return (srcdst_t) { SRCDST_BUF, { .buf = buf, .width = width, } };
}

INLINING srcdst_t dst_pkt(packet_descriptor_t* pd) {
    return (srcdst_t) { SRCDST_PKT, { .pd = pd } };
}

INLINING srcdst_t dst_handle(bitfield_handle_t fd) {
    return (srcdst_t) { SRCDST_HANDLE, { .fd = fd, } };
}

INLINING srcdst_t src_buf(void* buf, int width) {
    return (srcdst_t) { SRCDST_BUF, { .buf = buf, .width = width, } };
}

INLINING srcdst_t src_pkt(packet_descriptor_t* pd) {
    return dst_pkt(pd);
}

INLINING srcdst_t src_32(uint32_t value32) {
    return (srcdst_t) { SRCDST_32, { .value32 = value32, } };
}

INLINING srcdst_t src_handle(bitfield_handle_t fd) {
    return dst_handle(fd);
}



// Modify operations

//void MODIFY(srcdst_t desc, field_instance_e fld, srcdst_t src, endian_strategy_t strategy);

INLINING void print_set_fld(packet_descriptor_t* pd, field_instance_e fld, uint8_t* buf, int size) {
    #ifdef T4P4S_DEBUG
        int byte_width = (size+7)/8;
        uint32_t value32 = *(uint32_t*)buf;

        debug("    " T4LIT(=,field) " Set " T4LIT(%s,header) "." T4LIT(%s,field) "/" T4LIT(%d) "%s = " T4LIT(%d) " = " T4LIT(0x%0*x,bytes) "\n",
              header_instance_names[fld_infos[fld].header_instance],
              field_names[fld],
              size % 8 == 0 ? byte_width : size,
              size % 8 == 0 ? "B" : "b",
              value32,
              2 * byte_width,
              value32);
    #endif
}

INLINING header_instance_e get_hdr(field_instance_e fld) {
    return fld_infos[fld].header_instance;
}

INLINING void print_set_fld_buf(field_instance_e fld, uint8_t* buf, int size) {
    #ifdef T4P4S_DEBUG
        int byte_width = (size+7)/8;

        dbg_bytes(buf, byte_width, "    " T4LIT(=,field) " Set " T4LIT(%s,header) "." T4LIT(%s,field) "/" T4LIT(%d) "%s = ",
              header_instance_names[get_hdr(fld)],
              field_names[fld],
              size % 8 == 0 ? byte_width : size,
              size % 8 == 0 ? "B" : "b");
    #endif
}



// Extract statement

extern void transfer_to_egress(packet_descriptor_t* pd);

extern int get_egress_port(packet_descriptor_t* pd);
extern int get_ingress_port(packet_descriptor_t* pd);
extern void mark_to_drop(SHORT_STDPARAMS);

// Helpers


INLINING header_descriptor_t header_desc_buf(void* buf, int size) {
    return (header_descriptor_t) { -1, buf, -1, size };
}

INLINING hdr_info_t HDRINFOS(field_instance_e fld) {
    return hdr_infos[fld_infos[fld].header_instance];
}

INLINING bool FLD_IS_FIXED_WIDTH(field_instance_e fld) {
    bool no_vw = HDRINFOS(fld).var_width_field == -1;
    return no_vw || fld != HDRINFOS(fld).var_width_field;
}

INLINING bool FLD_IS_FIXED_POS(field_instance_e fld) {
    bool no_vw     = HDRINFOS(fld).var_width_field == -1;
    bool before_vw = fld <= HDRINFOS(fld).var_width_field;
    return no_vw || before_vw;
}

INLINING int FLD_BITWIDTH(header_descriptor_t hdesc, field_instance_e fld) {
    return FLD_IS_FIXED_WIDTH(fld) ? fld_infos[fld].size : hdesc.vw_size;
}

INLINING int FLD_BYTEOFFSET(header_descriptor_t hdesc, field_instance_e fld) {
    int vw_offset = FLD_IS_FIXED_POS(fld) ? 0 : (hdesc.vw_size / 8);
    return fld_infos[fld].byte_offset + vw_offset;
}

INLINING bitfield_handle_t handle(header_descriptor_t hdesc, field_instance_e fld) {
    int size = FLD_BITWIDTH(hdesc, fld);
    int bit_offset = fld_infos[fld].bit_offset;
    int byte_offset = FLD_BYTEOFFSET(hdesc, fld);
    return (bitfield_handle_t) {
        .pointer     = ((uint8_t*)hdesc.pointer) + byte_offset,
        .is_t4p4s_byte_order = HDRINFOS(fld).is_metadata,

        .bitwidth    = size,
        .bytewidth   = to_bytes(size + bit_offset%8),

        .bitoffset   = bit_offset,
        .byteoffset  = byte_offset,

        .bitcount    = size + bit_offset,
        .bytecount   = to_bytes(size + bit_offset),

        .mask        = fld_infos[fld].mask,
        .fixed_width = FLD_IS_FIXED_WIDTH(fld),

        .is_ok       = true,
    };
}

INLINING bool check_hdr_is_valid(packet_descriptor_t* pd, header_instance_e hdr, const char* fld_name, const char* operation_txt) {
    if (unlikely(!is_header_valid(hdr, pd))) {
        debug("   " T4LIT(!!,warning) " Trying to %s field " T4LIT(%s,warning) "." T4LIT(%s,field) " in " T4LIT(invalid header,warning) "\n", operation_txt, hdr_infos[hdr].name, fld_name);
        return false;
    }

    return true;
}



INLINING bitfield_handle_t get_handle_fld(packet_descriptor_t* pd, field_instance_e fld, const char* operation_txt) {
    header_instance_e hdr = to_hdr(fld);

    bool is_ok = check_hdr_is_valid(pd, hdr, field_names[fld], operation_txt);
    if (unlikely(!is_ok))   return (bitfield_handle_t) { .is_ok = false };

    int size = fld_infos[fld].is_vw ? pd->headers[hdr].vw_size : fld_infos[fld].size;
    return handle(header_desc_buf(pd->headers[hdr].pointer, size), fld);
}

INLINING bitfield_handle_t get_handle_buf(void* buf, int size, field_instance_e fld) {
    return handle(header_desc_buf(buf, size), fld);
}

INLINING bitfield_handle_t get_handle_32(uint32_t value32, field_instance_e fld) {
    return get_handle_buf(&value32, fld_infos[fld].byte_width, fld);
}


INLINING bitfield_handle_t get_handle(srcdst_t srcdst, field_instance_e fld, const char* operation_txt) {
    srcdst_type_t sd = srcdst.srcdst_type;
    return sd == SRCDST_BUF     ? get_handle_buf(srcdst.buf, srcdst.width, fld) :
           sd == SRCDST_PKT     ? get_handle_fld(srcdst.pd, fld, operation_txt) :
           sd == SRCDST_32      ? get_handle_32(srcdst.value32, fld)            :
              /* SRCDST_HANDLE */ srcdst.fd;
}

INLINING uint32_t FLD_MASK(bitfield_handle_t fd) {
    if (fd.fixed_width)  return fd.mask;

    uint32_t mask_offset = ~0 >> fd.bitoffset;
    uint32_t mask_len = ~0 << (32 - fd.bitcount);
    return mask_offset & mask_len;
}

INLINING uint32_t FLD_BYTES(bitfield_handle_t fd) {
    return fd.bytecount == 1 ? (*(uint8_t*)  fd.pointer) :
           fd.bytecount == 2 ? (*(uint16_t*) fd.pointer) :
                               (*(uint32_t*) fd.pointer);
}



INLINING uint32_t GET32_FLD_IMPL(bitfield_handle_t src) {
    if (src.is_t4p4s_byte_order)   return FLD_BYTES(src);

    uint32_t masked = net2t4p4s_4(FLD_BYTES(src)) & FLD_MASK(src);
    int downshift = 32 - src.bitoffset - src.bitwidth;
    return masked >> downshift;
}

// Extracts a field to the given destination [ONLY BYTE ALIGNED]
INLINING void GET_BUF_IMPL(void* dst, bitfield_handle_t src) {
    memcpy(dst, src.pointer, src.bytewidth);
}

uint32_t GET32_META_IMPL(bitfield_handle_t fd);

INLINING uint32_t GET32(srcdst_t src, field_instance_e fld) {
    bool is_meta = hdr_infos[fld_infos[fld].header_instance].is_metadata;
    int size = fld_infos[fld].size;
    bitfield_handle_t src_handle = get_handle(src, fld, "read");
    if (likely(src_handle.is_ok)) {
        return GET32_FLD_IMPL(src_handle);
    } else {
        // TODO 1. this should never happen
        //      2. if it does, drop the packet
        return 0xDEADC0DE;
    }
}


// works like GET32 unless the field's header is invalid, in which case it returns the default value
INLINING uint32_t GET32_def(srcdst_t src, field_instance_e fld, uint32_t default_value) {
    return is_header_valid(get_hdr(fld), src.pd) ? GET32(src, fld) : default_value;
}

INLINING void GET_BUF(void* dst_ptr, srcdst_t src, field_instance_e fld) {
    bitfield_handle_t src_handle = get_handle(src, fld, "read");
    GET_BUF_IMPL(dst_ptr, src_handle);
}


INLINING void MODIFY_BUF_IMPL(bitfield_handle_t dst, void* src, int src_bytewidth) {
    memset(dst.pointer, 0, dst.bytewidth - src_bytewidth);
    memcpy(dst.pointer + (dst.bytewidth - src_bytewidth), src, src_bytewidth);
}

INLINING uint32_t bitshift(int container_length, bitfield_handle_t fd) {
    return container_length - fd.bitoffset - fd.bitwidth;
}

INLINING uint32_t MASK_AT(int shift, uint32_t value32, uint32_t mask) {
    return ((value32 << shift) & mask) >> shift;
}

// Modifies a field in the packet by a uint32_t value with byte conversion (always) [MAX 4 BYTES]
INLINING void MODIFY32_T4P4S2NET_IMPL(bitfield_handle_t dst, uint32_t value32) {
    uint32_t res32 = FLD_BYTES(dst) & ~FLD_MASK(dst);
    res32 |= t4p4s2net(dst, value32 << (padded_bytecount(dst) - dst.bitcount)) & FLD_MASK(dst);
    memcpy(dst.pointer, &res32, dst.bytecount);
}

INLINING void MODIFY32_T4P4S_ORDER(bitfield_handle_t dst, uint32_t value32) {
    if      (dst.bytecount == 1)   *(uint8_t*)dst.pointer = (uint8_t)value32;
    else if (dst.bytecount == 1)  *(uint16_t*)dst.pointer = (uint16_t)value32;
    else                          *(uint32_t*)dst.pointer = value32;
}

INLINING void MODIFY32_IMPL_NET_ORDER(bitfield_handle_t dst, uint32_t value32) {
    int upshift = 32 - dst.bitoffset - dst.bitwidth;

    uint32_t old_content = t4p4s2net_4(*(uint32_t*)(dst.pointer)) & ~FLD_MASK(dst);
    uint32_t new_content = (value32 << upshift) & FLD_MASK(dst);

    *(uint32_t*)(dst.pointer) = net2t4p4s_4(old_content | new_content);
}

// Modifies a field in the packet by a uint32_t value with byte conversion when necessary [MAX 4 BYTES]
INLINING void MODIFY32_IMPL(bitfield_handle_t dst, uint32_t value32) {
    dst.is_t4p4s_byte_order ? MODIFY32_T4P4S_ORDER(dst, value32) : MODIFY32_IMPL_NET_ORDER(dst, value32);
}

// Modifies a field in the packet by the given source and length (byte conversion when necessary) [MAX 4 BYTES]
INLINING void MODIFY32_BUF_IMPL(bitfield_handle_t dst, void* src, int src_bytewidth) {
    uint32_t value32 = 0;
    memcpy(&value32, src, src_bytewidth);
    MODIFY32_IMPL(dst, value32);
}


INLINING void MODIFY(srcdst_t dst, field_instance_e fld, srcdst_t src, endian_strategy_t strategy) {
    if (fld_infos[fld].is_vw) {
        header_instance_e hdr = to_hdr(fld);
        dst.pd->headers[hdr].vw_size = src.width;
        bool is_byte_aligned = src.width % 8 == 0;
        debug("    : Update variable width field bit width: " T4LIT(%s,header) "." T4LIT(%s,field) "=" T4LIT(%d) "%s\n",
              header_instance_names[hdr], field_names[fld], src.width / (is_byte_aligned ? 8 : 1), is_byte_aligned ? "B" : "b");
    }

    bitfield_handle_t dst_handle = get_handle(dst, fld, "write");

    if (src.srcdst_type == SRCDST_BUF) {
        int src_bytewidth = (src.width + 7) / 8;
        switch (strategy) {
            case ENDIAN_NET:
                MODIFY_BUF_IMPL(dst_handle, src.buf, src_bytewidth);
                return;
            case ENDIAN_CONVERT_AS_NEEDED:
                MODIFY32_BUF_IMPL(dst_handle, src.buf, src_bytewidth);
                return;
            default: ; /* print warning at the end */
        }
    }

    if (src.srcdst_type == SRCDST_32) {
        switch (strategy) {
            case ENDIAN_KEEP: MODIFY32_IMPL(dst_handle, src.value32); return;
            case ENDIAN_NET:  MODIFY32_IMPL(dst_handle, src.value32); return;
            default: ; /* print warning at the end */
        }
    }


    #ifdef T4P4S_DEBUG
        static const char* endian_strategy_names[] = { "ENDIAN_CONVERT_ALWAYS", "ENDIAN_CONVERT_AS_NEEDED", "ENDIAN_KEEP", "ENDIAN_NET" };
        static const char* srcdst_type_names[]     = { "SRCDST_BUF", "SRCDST_PKT", "SRCDST_HANDLE", "SRCDST_32" };

        debug("    " T4LIT(!,warning) " Unknown options " T4LIT(%s;%s,warning) " for packet modification\n", srcdst_type_names[dst.srcdst_type], endian_strategy_names[strategy]);
    #endif
}


// #include "gen_model.h"
#include "dpdk_model_v1model.h"

INLINING void set_fld(packet_descriptor_t* pd, field_instance_e fld, uint32_t value32) {
    int size = fld_infos[fld].size;
    bool is_meta = hdr_infos[fld_infos[fld].header_instance].is_metadata;

    #ifdef T4P4S_DEBUG
        bool is_reviving_dropped = fld == EGRESS_META_FLD && get_egress_port(pd) == EGRESS_DROP_VALUE;
    #endif

    #ifdef T4P4S_DEBUG
        if (fld == EGRESS_META_FLD)   pd->is_egress_port_set = true;
    #endif

    print_set_fld(pd, fld, (uint8_t*)&value32, size);
    MODIFY(dst_pkt(pd), fld, src_32(value32), ENDIAN_KEEP);

    #ifdef T4P4S_DEBUG
        uint32_t value_back = GET32(src_pkt(pd), fld);
        if (value32 != value_back) {
            debug("       " T4LIT(!,warning) " Warning: written content was " T4LIT(%d) " (" T4LIT(%08x) ") but got " T4LIT(%d) " (" T4LIT(%08x) ") after read back\n",
                  value32, value32, value_back, value_back);
        }
    #endif

    #ifdef T4P4S_DEBUG
        if (is_reviving_dropped) {
            debug("       " T4LIT(!,warning) " Warning: packet was " T4LIT(dropped,status) " before%s\n",
                  value32 != EGRESS_DROP_VALUE ? ", now it is " T4LIT(active,status) " again" : "");
        }
    #endif
}

INLINING void set_fld_buf(packet_descriptor_t* pd, field_instance_e fld, uint8_t* buf) {
    int size = unlikely(fld_infos[fld].is_vw)
        ? pd->headers[to_hdr(fld)].vw_size
        : fld_infos[fld].size;

    print_set_fld_buf(fld, buf, size);
    MODIFY(dst_pkt(pd), fld, src_buf(buf, size), ENDIAN_NET);
}

