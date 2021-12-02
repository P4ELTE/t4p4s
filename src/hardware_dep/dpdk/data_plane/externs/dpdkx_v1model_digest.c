// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_v1model.h"
#include "util_packet.h"
#include "util_debug.h"

#include "dpdk_lib.h"
#include "stateful_memory.h"

#include <rte_ip.h>

#ifdef T4P4S_TYPE_arp_digest
void EXTERNIMPL1(digest,arp_digest)(uint32_t receiver, arp_digest_t* data, SHORT_STDPARAMS) {
    debug("    : Called EXTERNIMPL1(digest,arp_digest)\n")
}
#endif

#ifdef T4P4S_TYPE_mac_learn_digest
void EXTERNIMPL1(digest,mac_learn_digest)(uint32_t receiver, mac_learn_digest_t* data, SHORT_STDPARAMS) {
    debug("    : Called EXTERNIMPL1(digest,mac_learn_digest)\n")
}
#endif
