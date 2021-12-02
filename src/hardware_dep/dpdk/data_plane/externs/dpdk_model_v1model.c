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

void EXTERNIMPL1(digest,u8s)(uint32_t receiver, uint8_buffer_t buf, SHORT_STDPARAMS) {
    debug("    : TODO Sending digest\n");
}

void EXTERNIMPL0(clone)(enum_CloneType_t type, uint32_t session, SHORT_STDPARAMS) {
    debug("    : TODO Invoked clone, currently not implemented\n");
}

void EXTERNIMPL0(clone3)(enum_CloneType_t type, uint32_t session, uint8_buffer_t data, SHORT_STDPARAMS) {
    debug("    : TODO Invoked clone3, currently not implemented\n");
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

void EXTERNIMPL0(mark_to_drop)(SHORT_STDPARAMS) {
    MODIFY(dst_pkt(pd), EGRESS_META_FLD, src_32(EGRESS_DROP_VALUE), ENDIAN_KEEP);
    debug("       " T4LIT(X,status) " Packet is " T4LIT(dropped,status) "\n");
}
