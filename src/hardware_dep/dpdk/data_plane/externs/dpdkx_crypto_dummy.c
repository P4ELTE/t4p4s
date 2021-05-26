// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_lib.h"

void dummy_crypto(SHORT_STDPARAMS) {
    debug("    : Called extern dummy_crypto\n");
    debug_mbuf(pd->wrapper, "    : PD wrapper");
}

void dummy_crypto__u8s(uint8_buffer_t buf, SHORT_STDPARAMS) {
    debug("    : Called extern dummy_crypto__u8s: %d\n", ((uint8_t*)buf.buffer)[0]);
    debug_mbuf(pd->wrapper, "    : PD wrapper");
}

void dummy_crypto__u8s__u8s(uint8_buffer_t buf1, uint8_buffer_t buf2, SHORT_STDPARAMS) {
    debug("    : Called extern dummy_crypto__u8s__u8s: %d %d\n", ((uint8_t*)buf1.buffer)[0], ((uint8_t*)buf2.buffer)[0]);
    debug_mbuf(pd->wrapper, "    : PD wrapper");
}

void dummy_crypto__u16s__u32s(uint8_buffer_t buf1, uint8_buffer_t buf2, SHORT_STDPARAMS) {
    debug("    : Called extern dummy_crypto__u16s__u32s: %hd %d\n", ((uint16_t*)buf1.buffer)[0], ((uint32_t*)buf2.buffer)[0]);
    debug_mbuf(pd->wrapper, "    : PD wrapper");
}


void dummy_crypto__u32(uint8_buffer_t buf1, SHORT_STDPARAMS) {
    debug_mbuf(pd->wrapper, "    : PD wrapper");
}
