// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_psa.h"

#include <rte_ip.h>
#include "util_debug.h"

#include "dpdk_lib.h"


void InternetChecksum_t_init(InternetChecksum* checksum) {
    debug(" :::: calling extern " T4LIT(InternetChecksum_t_init,extern) "\n");
}

void extern_InternetChecksum_clear(InternetChecksum* checksum) {
    debug(" :::: calling extern " T4LIT(extern_InternetChecksum_clear,extern) "\n");
}

void extern_InternetChecksum_add(InternetChecksum* checksum) {
    debug(" :::: calling extern " T4LIT(extern_InternetChecksum_add,extern) "\n");
}

void InternetChecksum_t_get(InternetChecksum* checksum) {
    debug(" :::: calling extern " T4LIT(InternetChecksum_t_get,extern) "\n");
}

void InternetChecksum_t_subtract(InternetChecksum* checksum, uint16_t data) {
    debug(" :::: calling extern " T4LIT(InternetChecksum_t_subtract,extern) "\n");
}

uint16_t InternetChecksum_t_get_state(InternetChecksum* checksum) {
    debug(" :::: calling extern " T4LIT(InternetChecksum_t_get_state,extern) "\n");
}

void InternetChecksum_t_set_state(InternetChecksum* checksum, uint16_t checksum_state) {
    debug(" :::: calling extern " T4LIT(InternetChecksum_t_set_state,extern) "\n");
}
