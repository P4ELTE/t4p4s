// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_psa.h"

#include <rte_ip.h>
#include "util_debug.h"

#include "dpdk_lib.h"


void EXTERNIMPL0(InternetChecksum,init)(InternetChecksum* checksum, SHORT_STDPARAMS) {
    debug(" :::: calling extern " T4LIT(InternetChecksum/init,extern) "\n");
}

void EXTERNIMPL0(InternetChecksum,clear)(InternetChecksum* checksum, SHORT_STDPARAMS) {
    debug(" :::: calling extern " T4LIT(InternetChecksum/clear,extern) "\n");
}

void EXTERNIMPL0(InternetChecksum,add)(InternetChecksum* checksum) {
    debug(" :::: calling extern " T4LIT(InternetChecksum/add,extern) "\n");
}

void EXTERNIMPL0(InternetChecksum,get)(InternetChecksum* checksum, SHORT_STDPARAMS) {
    debug(" :::: calling extern " T4LIT(InternetChecksum/get,extern) "\n");
}

void EXTERNIMPL0(InternetChecksum,subtract)(InternetChecksum* checksum, uint16_t data, SHORT_STDPARAMS) {
    debug(" :::: calling extern " T4LIT(InternetChecksum/subtract,extern) "\n");
}

uint16_t EXTERNIMPL0(InternetChecksum,get_state)(InternetChecksum* checksum) {
    debug(" :::: calling extern " T4LIT(InternetChecksum/get_state,extern) "\n");
}

void EXTERNIMPL0(InternetChecksum,set_state)(InternetChecksum* checksum, uint16_t checksum_state, SHORT_STDPARAMS) {
    debug(" :::: calling extern " T4LIT(InternetChecksum/set_state,extern) "\n");
}
