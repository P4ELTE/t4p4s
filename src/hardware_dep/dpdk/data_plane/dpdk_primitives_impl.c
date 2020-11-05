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
