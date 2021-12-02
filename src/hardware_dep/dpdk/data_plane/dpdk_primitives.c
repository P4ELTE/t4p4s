// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#ifdef T4P4S_UNITTEST
    #include "t4p4s_unittest_lib.h"
    #include "util_packet_bitfield.h"
    #include "dpdk_primitives.h"
    #include "dataplane_hdr_fld_pkt.h"
#else
    #include "dpdk_primitives_impl.h"
    #ifdef T4P4S_DEBUG
        #include "util_debug.h"
    #endif
#endif

srcdst_t dst_buf(void* buf, int width) {
    return (srcdst_t) { SRCDST_BUF, { .buf = buf, .width = width, } };
}

srcdst_t dst_pkt(packet_descriptor_t* pd) {
    return (srcdst_t) { SRCDST_PKT, { .pd = pd } };
}

srcdst_t dst_handle(bitfield_handle_t fd) {
    return (srcdst_t) { SRCDST_HANDLE, { .fd = fd, } };
}

srcdst_t src_buf(void* buf, int width) {
    return (srcdst_t) { SRCDST_BUF, { .buf = buf, .width = width, } };
}

srcdst_t src_pkt(packet_descriptor_t* pd) {
    return dst_pkt(pd);
}

srcdst_t src_32(uint32_t value32) {
    return (srcdst_t) { SRCDST_32, { .value32 = value32, } };
}

srcdst_t src_handle(bitfield_handle_t fd) {
    return dst_handle(fd);
}


bitfield_handle_t get_handle(srcdst_t srcdst, field_instance_e fld, const char* operation_txt) {
    srcdst_type_t sd = srcdst.srcdst_type;
    return sd == SRCDST_BUF     ? get_handle_buf(srcdst.buf, srcdst.width, fld) :
           sd == SRCDST_PKT     ? get_handle_fld(srcdst.pd, fld, operation_txt) :
           sd == SRCDST_32      ? get_handle_32(srcdst.value32, fld)            :
              /* SRCDST_HANDLE */ srcdst.fd;
}


header_instance_e get_hdr(field_instance_e fld) {
    return fld_infos[fld].header_instance;
}


// Extract operations

uint32_t GET32(srcdst_t src, field_instance_e fld) {
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
uint32_t GET32_def(srcdst_t src, field_instance_e fld, uint32_t default_value) {
    return is_header_valid(get_hdr(fld), src.pd) ? GET32(src, fld) : default_value;
}

void GET_BUF(void* dst_ptr, srcdst_t src, field_instance_e fld) {
    bitfield_handle_t src_handle = get_handle(src, fld, "read");
    GET_BUF_IMPL(dst_ptr, src_handle);
}


// Modify operations
extern void MODIFY_BUF_IMPL(bitfield_handle_t dst, void* src, int src_bytewidth);
extern void MODIFY32_BUF_IMPL(bitfield_handle_t dst, void* src, int src_bytewidth);
extern void MODIFY32_IMPL(bitfield_handle_t dst, uint32_t value32);

void MODIFY(srcdst_t dst, field_instance_e fld, srcdst_t src, endian_strategy_t strategy) {
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

void print_set_fld(packet_descriptor_t* pd, field_instance_e fld, uint8_t* buf, int size) {
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

void print_set_fld_buf(field_instance_e fld, uint8_t* buf, int size) {
    #ifdef T4P4S_DEBUG
        int byte_width = (size+7)/8;

        dbg_bytes(buf, byte_width, "    " T4LIT(=,field) " Set " T4LIT(%s,header) "." T4LIT(%s,field) "/" T4LIT(%d) "%s = ",
              header_instance_names[get_hdr(fld)],
              field_names[fld],
              size % 8 == 0 ? byte_width : size,
              size % 8 == 0 ? "B" : "b");
    #endif
}

void set_fld(packet_descriptor_t* pd, field_instance_e fld, uint32_t value32) {
    int size = fld_infos[fld].size;

    print_set_fld(pd, fld, (uint8_t*)&value32, size);
    MODIFY(dst_pkt(pd), fld, src_32(value32), ENDIAN_KEEP);
}

void set_fld_buf(packet_descriptor_t* pd, field_instance_e fld, uint8_t* buf) {
    int size = unlikely(fld_infos[fld].is_vw)
        ? pd->headers[to_hdr(fld)].vw_size
        : fld_infos[fld].size;

    print_set_fld_buf(fld, buf, size);
    MODIFY(dst_pkt(pd), fld, src_buf(buf, size), ENDIAN_NET);
}

// Helpers

header_descriptor_t header_desc_buf(void* buf, int size) {
    return (header_descriptor_t) { -1, buf, -1, size };
}

hdr_info_t HDRINFOS(field_instance_e fld) {
    return hdr_infos[fld_infos[fld].header_instance];
}

bool FLD_IS_FIXED_WIDTH(field_instance_e fld) {
    bool no_vw = HDRINFOS(fld).var_width_field == -1;
    return no_vw || fld != HDRINFOS(fld).var_width_field;
}

bool FLD_IS_FIXED_POS(field_instance_e fld) {
    bool no_vw     = HDRINFOS(fld).var_width_field == -1;
    bool before_vw = fld <= HDRINFOS(fld).var_width_field;
    return no_vw || before_vw;
}

int FLD_BITWIDTH(header_descriptor_t hdesc, field_instance_e fld) {
    return FLD_IS_FIXED_WIDTH(fld) ? fld_infos[fld].size : hdesc.vw_size;
}

int FLD_BYTEOFFSET(header_descriptor_t hdesc, field_instance_e fld) {
    int vw_offset = FLD_IS_FIXED_POS(fld) ? 0 : (hdesc.vw_size / 8);
    return fld_infos[fld].byte_offset + vw_offset;
}

bitfield_handle_t handle(header_descriptor_t hdesc, field_instance_e fld) {
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

bool check_hdr_is_valid(packet_descriptor_t* pd, header_instance_e hdr, const char* fld_name, const char* operation_txt) {
    if (unlikely(!is_header_valid(hdr, pd))) {
        debug("   " T4LIT(!!,warning) " Trying to %s field " T4LIT(%s,warning) "." T4LIT(%s,field) " in " T4LIT(invalid header,warning) "\n", operation_txt, hdr_infos[hdr].name, fld_name);
        return false;
    }
    
    return true;
}

bitfield_handle_t get_handle_fld(packet_descriptor_t* pd, field_instance_e fld, const char* operation_txt) {
    header_instance_e hdr = to_hdr(fld);

    bool is_ok = check_hdr_is_valid(pd, hdr, field_names[fld], operation_txt);
    if (unlikely(!is_ok))   return (bitfield_handle_t) { .is_ok = false };

    int size = fld_infos[fld].is_vw ? pd->headers[hdr].vw_size : fld_infos[fld].size;
    return handle(header_desc_buf(pd->headers[hdr].pointer, size), fld);
}

bitfield_handle_t get_handle_buf(void* buf, int size, field_instance_e fld) {
    return handle(header_desc_buf(buf, size), fld);
}

bitfield_handle_t get_handle_32(uint32_t value32, field_instance_e fld) {
    return get_handle_buf(&value32, fld_infos[fld].byte_width, fld);
}
