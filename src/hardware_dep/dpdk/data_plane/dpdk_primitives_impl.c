// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_lib.h"
#include "util_debug.h"


void check_hdr_is_valid(packet_descriptor_t* pd, header_instance_t hdr, const char* operation_txt) {
    if (unlikely(pd->headers[hdr].pointer == 0)) {
        debug("Cannot %s header " T4LIT(%s,header) ", as it is " T4LIT(invalid,warning) "\n", operation_txt, hdr_infos[hdr].name);
    }
}

/*void GET_INT32_AUTO_PACKET(packet_descriptor_t* pd, header_instance_t hdr, field_instance_t fld) {
    check_hdr_is_valid(pd, hdr, "get");
    GET_INT32_AUTO(handle(header_desc_ins(pd, hdr), fld));
}

void EXTRACT_BYTEBUF_PACKET(packet_descriptor_t* pd, header_instance_t hdr, field_instance_t fld, void* dst) {
    check_hdr_is_valid(pd, hdr, "read");
    EXTRACT_BYTEBUF(handle(header_desc_ins(pd, hdr), fld), dst);
}

void EXTRACT_INT32_AUTO_PACKET(packet_descriptor_t* pd, header_instance_t hdr, field_instance_t fld, void* dst) {
    check_hdr_is_valid(pd, hdr, "read");
    EXTRACT_INT32_AUTO(handle(header_desc_ins(pd, hdr), fld), dst);
}

void EXTRACT_INT32_BITS_PACKET(packet_descriptor_t* pd, header_instance_t hdr, field_instance_t fld, void* dst) {
    check_hdr_is_valid(pd, hdr, "read");
    EXTRACT_INT32_BITS(handle(header_desc_ins(pd, hdr), fld), dst);
}
*/

void MODIFY_BYTEBUF_BYTEBUF_PACKET(packet_descriptor_t* pd, header_instance_t hdr, field_instance_t fld, void* src, int srclen) {
    check_hdr_is_valid(pd, hdr, "read");
    MODIFY_BYTEBUF_BYTEBUF(handle(header_desc_ins(pd, hdr), fld), src, srclen);
}

/*void MODIFY_INT32_BYTEBUF_PACKET(packet_descriptor_t* pd, header_instance_t hdr, field_instance_t fld, void* src, int srclen) {
    check_hdr_is_valid(pd, hdr, "read");
    MODIFY_INT32_BYTEBUF(handle(header_desc_ins(pd, hdr), fld), src, srclen);
}

void MODIFY_INT32_INT32_BITS_PACKET(packet_descriptor_t* pd, header_instance_t hdr, field_instance_t fld, uint32_t value32) {
    check_hdr_is_valid(pd, hdr, "read");
    MODIFY_INT32_INT32_BITS(handle(header_desc_ins(pd, hdr), fld), value32);
}
*/

// TODO simplify all other interface macros, too
void MODIFY_INT32_INT32_AUTO_PACKET(packet_descriptor_t* pd, header_instance_t h, field_instance_t f, uint32_t value32) {
    MODIFY_INT32_INT32_AUTO(handle(header_desc_ins(pd, h), f), value32);
}


void set_field(fldT f[], bufT b[], uint32_t value32, int bit_width) {
#ifdef T4P4S_DEBUG
    // exactly one of `f` and `b` have to be non-zero
    assert((f == 0) != (b == 0));
#endif

    if (f != 0) {
        fldT fld = f[0];
        int byte_width = (bit_width+7)/8;

        debug("    " T4LIT(=,field) " Set " T4LIT(%s,header) "." T4LIT(%s,field) "/" T4LIT(%d) "b (" T4LIT(%d) "B) = " T4LIT(%d) " (0x" T4LIT(%0*x) ")\n",
              header_instance_names[fld.hdr],
              field_names[fld.fld],
              bit_width,
              byte_width,
              value32,
              2 * byte_width,
              value32);

        MODIFY_INT32_INT32_AUTO_PACKET(fld.pd, fld.hdr, fld.fld, value32);
    }

    // TODO implement this case, too
    if (b != 0)   rte_exit(2, "TODO unimplemented portion of set_field");
}
