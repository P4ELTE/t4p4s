// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_lib.h"

void EXTERNIMPL0(dummy_crypto)(SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(dummy_crypto,extern) "\n");
}

void EXTERNIMPL1(dummy_crypto,u8)(uint8_t param, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(dummy_crypto/u8,extern) "\n");
}

void EXTERNIMPL1(dummy_crypto,u16)(uint16_t param, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(dummy_crypto/u16,extern) "\n");
}

void EXTERNIMPL1(dummy_crypto,u32)(uint32_t param, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(dummy_crypto/u32,extern) "\n");
}

void EXTERNIMPL1(dummy_crypto,buf)(uint8_t* param, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(dummy_crypto/buf,extern) "\n");
}

void EXTERNIMPL1(dummy_crypto,u8s)(uint8_buffer_t param, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(dummy_crypto/u8s,extern) "\n");
}

void EXTERNIMPL2(dummy_crypto,u8s,u8s)(uint8_buffer_t param1, uint8_buffer_t param2, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(dummy_crypto/u8s/u8s,extern) "\n");
}
