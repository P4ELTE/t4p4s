// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_lib.h"

void dummy_crypto(SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(dummy_crypto,extern) "\n");
}

void dummy_crypto__u8s(uint8_buffer_t buf, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(dummy_crypto__u8s,extern) ": " T4LIT(%d) "\n", ((uint8_t*)buf.buffer)[0]);
}

void dummy_crypto__u8s__u8s(uint8_buffer_t buf1, uint8_buffer_t buf2, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(dummy_crypto__u8s__u8s,extern) ": " T4LIT(%d) " " T4LIT(%d) "\n", ((uint8_t*)buf1.buffer)[0], ((uint8_t*)buf2.buffer)[0]);
}

void dummy_crypto__u16s__u32s(uint8_buffer_t buf1, uint8_buffer_t buf2, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(dummy_crypto__u16s__u32s,extern) ": " T4LIT(%hd) " " T4LIT(%d) "\n", ((uint16_t*)buf1.buffer)[0], ((uint32_t*)buf2.buffer)[0]);
}


void dummy_crypto__u8(uint8_t u8, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(dummy_crypto__u8,extern) ": " T4LIT(%d) " (" T4LIT(0x%02x,bytes) ")\n", u8, u8);
}

void dummy_crypto__u16(uint16_t u16, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(dummy_crypto__u16,extern) ": " T4LIT(%d) " (" T4LIT(0x%04x,bytes) ")\n", u16, u16);
}

void dummy_crypto__u32(uint32_t u32, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(dummy_crypto__u32,extern) ": " T4LIT(%d) " (" T4LIT(0x%08x,bytes) ")\n", u32, u32);
}

void dummy_crypto__buf(uint8_t* u8s, SHORT_STDPARAMS) {
    // TODO for now, we assume that the buffer is 64 bits long
    int len = 64/8;
    dbg_bytes(u8s, len, "    : Called extern " T4LIT(dummy_crypto__buf,extern) ": ");
}
