// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_lib.h"

void dummy_crypto_impl(uint8_buffer_t buf, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(dummy_crypto,extern) "\n");
}
