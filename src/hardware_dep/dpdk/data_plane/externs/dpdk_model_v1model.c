// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_v1model.h"
#include "util_packet.h"
#include "util_debug.h"

#include "dpdk_lib.h"
#include "stateful_memory.h"

#include <rte_ip.h>

#define UNUSED_FIELD_IDX -1

// -----------------------------------------------------------------

void transfer_to_egress(packet_descriptor_t* pd) {
}

// -----------------------------------------------------------------

void digest_impl(int receiver, uint8_buffer_t buf, SHORT_STDPARAMS) {
    
}

void clone_impl(enum_CloneType_t type, uint32_t session, SHORT_STDPARAMS) {
    
}

void clone3_impl(enum_CloneType_t type, uint32_t session, uint8_buffer_t data, SHORT_STDPARAMS) {
    
}

// -----------------------------------------------------------------

int get_egress_port(packet_descriptor_t* pd) {
    return GET32(src_pkt(pd), EGRESS_META_FLD);
}

int get_ingress_port(packet_descriptor_t* pd) {
    return GET32(src_pkt(pd), INGRESS_META_FLD);
}

// -----------------------------------------------------------------

void set_handle_packet_metadata(packet_descriptor_t* pd, uint32_t portid)
{
    MODIFY(dst_pkt(pd), INGRESS_META_FLD, src_32(portid), ENDIAN_KEEP);
}

// -----------------------------------------------------------------

void mark_to_drop_impl(SHORT_STDPARAMS) {
    MODIFY(dst_pkt(pd), EGRESS_META_FLD, src_32(EGRESS_DROP_VALUE), ENDIAN_KEEP);
    debug("       " T4LIT(X,status) " Packet is " T4LIT(dropped,status) "\n");
}
