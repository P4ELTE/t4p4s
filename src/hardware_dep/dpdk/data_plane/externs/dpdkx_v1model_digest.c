// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_v1model.h"
#include "util_packet.h"
#include "util_debug.h"

#include "dpdk_lib.h"
#include "stateful_memory.h"

#include <rte_ip.h>

bool encountered_digest_without_control_plane = false;

void EXTERNIMPL0(digest)(uint32_t receiver, uint8_buffer_t buf, SHORT_STDPARAMS) {
    debug("    : Called EXTERNIMPL0(digest)\n")
}
