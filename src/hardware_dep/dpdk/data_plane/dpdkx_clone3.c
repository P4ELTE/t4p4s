// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_v1model.h"
#include "util_packet.h"
#include "util_debug.h"

#include "dpdk_lib.h"
#include "stateful_memory.h"

#include <rte_ip.h>

void clone3(enum_CloneType_t type, uint32_t session, uint8_buffer_t data, SHORT_STDPARAMS) {
    debug("    : Executing clone3\n");
}
