// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_psa.h"

#include <rte_ip.h>
#include "actions.h"
#include "util_debug.h"

#include "dpdk_lib.h"
#include "stateful_memory.h"

extern struct all_metadatas_t all_metadatas;

void transfer_to_egress(packet_descriptor_t* pd)
{
    /*not implemented*/
}

int get_egress_port(packet_descriptor_t* pd) {
    return GET32(src_pkt(pd), FLD(all_metadatas,drop)) == true ? (PortId_t)EGRESS_DROP_VALUE : GET32(src_pkt(pd), EGRESS_META_FLD);
}

int get_ingress_port(packet_descriptor_t* pd) {
    return GET32(src_pkt(pd), INGRESS_META_FLD);
}

void set_handle_packet_metadata(packet_descriptor_t* pd, uint32_t portid)
{
    MODIFY(dst_pkt(pd), INGRESS_META_FLD, src_32(portid), ENDIAN_KEEP);
}

void mark_to_drop(SHORT_STDPARAMS) {
    MODIFY(dst_pkt(pd), EGRESS_META_FLD, src_32(EGRESS_DROP_VALUE), ENDIAN_KEEP);
    debug("       : " T4LIT(all_metadatas,header) "." T4LIT(EGRESS_META_FLD,field) " = " T4LIT(EGRESS_DROP_VALUE,bytes) "\n");
}

// -----------------------------------------------------------------

// PSA ports are 32b
// Sometimes the full range is used, sometimes, a small port number is chosen.
int pick_random_port() {
    if (rand() % 4 == 0)    return rand() % 4;
    if (rand() % 2 == 0)    return rand() % 1000;
    return rand();
}
