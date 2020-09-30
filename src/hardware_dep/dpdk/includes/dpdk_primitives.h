// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#pragma once

// Function based set field

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

static int set_field(fldT f[], bufT b[], uint32_t value32, int bit_width);

#include "dpdk_primitives_impl.h"


// Extract operations

#define GET_INT32_AUTO_PACKET(pd , h, f) GET_INT32_AUTO(handle(header_desc_ins(pd , h), f))
#define GET_INT32_AUTO_BUFFER(buf, w, f) GET_INT32_AUTO(handle(header_desc_buf(buf, w), f))

#define EXTRACT_BYTEBUF_PACKET(pd , h, f, dst) EXTRACT_BYTEBUF(handle(header_desc_ins(pd , h), f), dst)
#define EXTRACT_BYTEBUF_BUFFER(buf, w, f, dst) EXTRACT_BYTEBUF(handle(header_desc_buf(buf, w), f), dst)

#define EXTRACT_INT32_AUTO_PACKET(pd , h, f, dst) EXTRACT_INT32_AUTO(handle(header_desc_ins(pd , h), f), dst)
#define EXTRACT_INT32_AUTO_BUFFER(buf, w, f, dst) EXTRACT_INT32_AUTO(handle(header_desc_buf(buf, w), f), dst)

#define EXTRACT_INT32_BITS_PACKET(pd , h, f, dst) EXTRACT_INT32_BITS(handle(header_desc_ins(pd , h), f), dst)
#define EXTRACT_INT32_BITS_BUFFER(buf, w, f, dst) EXTRACT_INT32_BITS(handle(header_desc_buf(buf, w), f), dst)

// Modify operations

#define MODIFY_BYTEBUF_BYTEBUF_PACKET(pd , h, f, src, srclen) MODIFY_BYTEBUF_BYTEBUF(handle(header_desc_ins(pd , h), f), src, srclen);
#define MODIFY_BYTEBUF_BYTEBUF_BUFFER(buf, w, f, src, srclen) MODIFY_BYTEBUF_BYTEBUF(handle(header_desc_buf(buf, w), f), src, srclen);

#define MODIFY_INT32_BYTEBUF_PACKET(pd , h, f, src, srclen) MODIFY_INT32_BYTEBUF(handle(header_desc_ins(pd , h), f), src, srclen);
#define MODIFY_INT32_BYTEBUF_BUFFER(buf, w, f, src, srclen) MODIFY_INT32_BYTEBUF(handle(header_desc_buf(buf, w), f), src, srclen);

#define MODIFY_INT32_INT32_BITS_PACKET(pd , h, f, value32) MODIFY_INT32_INT32_BITS(handle(header_desc_ins(pd , h), f), value32);
#define MODIFY_INT32_INT32_BITS_BUFFER(buf, w, f, value32) MODIFY_INT32_INT32_BITS(handle(header_desc_buf(buf, w), f), value32);

#define MODIFY_INT32_INT32_AUTO_BUFFER(buf, w, f, value32) MODIFY_INT32_INT32_AUTO(handle(header_desc_buf(buf, w), f), value32);

// Extract statement

void transfer_to_egress(packet_descriptor_t* pd);

// Extract statement

int extract_egress_port(packet_descriptor_t* pd);
int extract_ingress_port(packet_descriptor_t* pd);
