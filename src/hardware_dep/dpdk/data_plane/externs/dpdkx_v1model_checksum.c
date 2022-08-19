// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_v1model.h"
#include "util_packet.h"
#include "util_debug.h"

#include "dpdk_lib.h"
#include "stateful_memory.h"

#include <rte_ip.h>

void EXTERNIMPL0(verify_checksum)(bool cond, uint8_buffer_t data, uint16_t checksum, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS) {
    if (unlikely(!cond)) {
        debug("    " T4LIT(X,status) " Packet checksum does not need to be verified\n");
        return;
    }

    int bytelen = data.size / 8;

    dbg_bytes(data.buffer, bytelen, "    : " T4LIT(Verifying checksum,extern) " for " T4LIT(%d) " bytes: ", bytelen);
    uint32_t current_cksum = 0;
    uint32_t calculated_cksum = 0;

    if (algorithm == enum_HashAlgorithm_csum16) {
        calculated_cksum = rte_raw_cksum(data.buffer, bytelen);
        calculated_cksum = (calculated_cksum == 0xffff) ? calculated_cksum : ((~calculated_cksum) & 0xffff);
    }

    if (unlikely(calculated_cksum != current_cksum)) {
        MODIFY(dst_pkt(pd), FLD(all_metadatas,checksum_error), src_32(true), ENDIAN_KEEP);
        debug("    " T4LIT(!,warning) " Packet checksum is " T4LIT(wrong,warning) ": " T4LIT(%04x,bytes) ", calculated checksum is " T4LIT(%04x,bytes) "\n", current_cksum, calculated_cksum);
    } else {
        debug("      : Packet checksum is " T4LIT(ok,success) ": " T4LIT(%04x,bytes) "\n", current_cksum);
    }
}

void EXTERNIMPL0(update_checksum)(bool cond, uint8_buffer_t data, uint16_t* checksum, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS) {
    if (unlikely(!cond)) {
        debug("    " T4LIT(X,status) " Packet checksum does not need to be verified\n");
        return;
    }

    int bytelen = data.size;

    dbg_bytes(data.buffer, bytelen, "    : " T4LIT(Updating checksum,extern) " for " T4LIT(%d) " bytes: ", bytelen);

    uint32_t calculated_cksum = 0;

    if (algorithm == enum_HashAlgorithm_csum16) {
	uint16_t calculated_cksum = 0; 
        calculated_cksum = rte_raw_cksum(data.buffer, bytelen);
        calculated_cksum = (calculated_cksum == 0xffff) ? calculated_cksum : ((~calculated_cksum) & 0xffff);

        debug("       : Packet checksum " T4LIT(updated,status) " to " T4LIT(%04x,bytes) "\n", calculated_cksum);
        *checksum = calculated_cksum;
    }
}

bool is_checksum_bad(struct rte_mbuf* mbuf) {
    #if RTE_VERSION >= RTE_VERSION_NUM(21,11,0,0)
        return (mbuf->ol_flags & RTE_MBUF_F_RX_IP_CKSUM_MASK) == RTE_MBUF_F_RX_IP_CKSUM_BAD;
    #else
        return (mbuf->ol_flags & PKT_RX_IP_CKSUM_MASK) == PKT_RX_IP_CKSUM_BAD;
    #endif
}

void EXTERNIMPL2(verify_checksum_offload,u8s,u16)(uint8_buffer_t data, enum_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(verify_checksum_offload,extern) "\n");

    if (unlikely(is_checksum_bad(pd->wrapper))) {
        // TODO
    }
}

void EXTERNIMPL0(verify_checksum_offload)(enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(verify_checksum_offload,extern) "\n");

    if (unlikely(is_checksum_bad(pd->wrapper))) {
        // TODO
    }
}

void update_checksum_offload(uint8_buffer_t data, enum_HashAlgorithm_t algorithm, uint8_t len_l2, uint8_t len_l3, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(update_checksum_offload,extern) "\n");

    pd->wrapper->l2_len = len_l2;
    pd->wrapper->l3_len = len_l3;

    #if RTE_VERSION >= RTE_VERSION_NUM(21,11,0,0)
        pd->wrapper->ol_flags |= RTE_MBUF_F_TX_IPV4 | RTE_MBUF_F_TX_IPV4;
    #else
        pd->wrapper->ol_flags |= PKT_TX_IPV4 | PKT_TX_IP_CKSUM;
    #endif

    // TODO use proper SET
    // MODIFY_INT32_INT32_BITS(cksum_field_handle, 0);

    debug("       : Updating packet checksum (offload)\n");
    // TODO implement offload
}


void EXTERNIMPL0(verify)(bool check, error_error_t toSignal, SHORT_STDPARAMS) {
    // TODO implement call to extern
    debug("    : Called extern " T4LIT(verify,extern) "\n");
}

void EXTERNIMPL0(verify_checksum_with_payload)(bool condition, uint8_buffer_t data, uint16_t checksum, enum_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    // TODO implement call to extern
    debug("    : Called extern " T4LIT(verify_checksum_with_payload,extern) "\n");
}

void EXTERNIMPL0(update_checksum_with_payload)(bool condition, uint8_buffer_t data, uint16_t* checksum, enum_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    // TODO implement call to extern
    debug("    : Called extern " T4LIT(update_checksum_with_payload,extern) "\n");
}

// ----------------------------------------------------------------

void EXTERNIMPL1(verify_checksum,u8s)(bool cond, uint8_buffer_t data, uint16_t checksum, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS) {
    EXTERNIMPL0(verify_checksum)(cond, data, checksum, algorithm, SHORT_STDPARAMS_IN);
}

void EXTERNIMPL1(update_checksum,u8s)(bool cond, uint8_buffer_t data, uint16_t* checksum, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS) {
    EXTERNIMPL0(update_checksum)(cond, data, checksum, algorithm, SHORT_STDPARAMS_IN);
}

void EXTERNIMPL2(verify_checksum_with_payload,u8s,u16)(bool condition, uint8_buffer_t data, uint16_t checksum, enum_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    EXTERNIMPL0(verify_checksum_with_payload)(condition, data, checksum, algo, SHORT_STDPARAMS_IN);
}

void EXTERNIMPL2(update_checksum_with_payload,u8s,u16)(bool condition, uint8_buffer_t data, uint16_t* /* inout */  checksum, enum_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    EXTERNIMPL0(update_checksum_with_payload)(condition, data, checksum, algo, SHORT_STDPARAMS_IN);
}

void EXTERNIMPL2(verify_checksum,u8s,u16)(bool condition, uint8_buffer_t data, uint16_t checksum, enum_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    EXTERNIMPL0(verify_checksum)(condition, data, checksum, algo, SHORT_STDPARAMS_IN);
}

void EXTERNIMPL2(verify_checksum,u32s,u16)(bool condition, uint8_buffer_t data, uint16_t checksum, enum_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    EXTERNIMPL0(verify_checksum)(condition, data, checksum, algo, SHORT_STDPARAMS_IN);
}

void EXTERNIMPL2(update_checksum,u8s,u16)(bool condition, uint8_buffer_t data, uint16_t* /* inout */ checksum, enum_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    EXTERNIMPL0(update_checksum)(condition, data, checksum, algo, SHORT_STDPARAMS_IN);
}

void EXTERNIMPL2(update_checksum,u8s,u32)(bool condition, uint8_buffer_t data, uint32_t* /* inout */ checksum, enum_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    EXTERNIMPL0(update_checksum)(condition, data, (uint16_t*)checksum, algo, SHORT_STDPARAMS_IN);
}

void EXTERNIMPL2(update_checksum,u32s,u16)(bool condition, uint8_buffer_t data, uint16_t* /* inout */ checksum, enum_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    EXTERNIMPL0(update_checksum)(condition, data, checksum, algo, SHORT_STDPARAMS_IN);
}

void EXTERNIMPL2(verify_checksum_with_payload,u32s,u16)(bool condition, uint8_buffer_t data, uint16_t checksum, enum_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    EXTERNIMPL0(verify_checksum_with_payload)(condition, data, checksum, algo, SHORT_STDPARAMS_IN);
}

void EXTERNIMPL2(update_checksum_with_payload,u32s,u16)(bool condition, uint8_buffer_t data, uint16_t* /* inout */ checksum, enum_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    EXTERNIMPL0(update_checksum_with_payload)(condition, data, checksum, algo, SHORT_STDPARAMS_IN);
}
