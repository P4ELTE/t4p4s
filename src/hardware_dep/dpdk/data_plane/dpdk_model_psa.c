// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_psa.h"

#include <rte_ip.h>
#include "actions.h"
#include "util_debug.h"

extern struct all_metadatas_t all_metadatas;

void InternetChecksum_t_init() {
    debug(" :::: calling extern " T4LIT(InternetChecksum_t_init,extern) "\n");
}

void extern_InternetChecksum_clear() {
    debug(" :::: calling extern " T4LIT(extern_InternetChecksum_clear,extern) "\n");
}

void extern_InternetChecksum_add() {
    debug(" :::: calling extern " T4LIT(extern_InternetChecksum_add,extern) "\n");
}

void InternetChecksum_t_get() {
    debug(" :::: calling extern " T4LIT(InternetChecksum_t_get,extern) "\n");
}

void transfer_to_egress(packet_descriptor_t* pd)
{
    /*not implemented*/
}

int extract_egress_port(packet_descriptor_t* pd) {
    return GET_INT32_AUTO_PACKET(pd, HDR(all_metadatas), EGRESS_META_FLD);
}

int extract_ingress_port(packet_descriptor_t* pd) {
    return GET_INT32_AUTO_PACKET(pd, HDR(all_metadatas), INGRESS_META_FLD);
}

void set_handle_packet_metadata(packet_descriptor_t* pd, uint32_t portid)
{
    int res32; // needed for the macro
    MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), INGRESS_META_FLD, portid);
}

void verify_checksum(bool cond, struct uint8_buffer_s data, bitfield_handle_t cksum_field_handle, enum_PSA_HashAlgorithm_t algorithm, SHORT_STDPARAMS) {
    debug(" :::: Calling extern " T4LIT(verify_checksum,extern) "\n");
}

void update_checksum(bool cond, struct uint8_buffer_s data, bitfield_handle_t cksum_field_handle, enum_PSA_HashAlgorithm_t algorithm, SHORT_STDPARAMS) {
    debug(" :::: Calling extern " T4LIT(update_checksum,extern) "\n");
}

void verify_checksum_offload(bitfield_handle_t cksum_field_handle, enum_PSA_HashAlgorithm_t algorithm, SHORT_STDPARAMS) {
    debug(" :::: Calling extern " T4LIT(verify_checksum_offload,extern) "\n");
}

void update_checksum_offload(bitfield_handle_t cksum_field_handle, enum_PSA_HashAlgorithm_t algorithm, uint8_t len_l2, uint8_t len_l3, SHORT_STDPARAMS) {
    debug(" :::: Calling extern " T4LIT(update_checksum_offload,extern) "\n");
}

void mark_to_drop(SHORT_STDPARAMS) {
    debug(" :::: Calling extern " T4LIT(mark_to_drop,extern) "\n");

    uint32_t res32;
    MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), EGRESS_META_FLD, EGRESS_DROP_VALUE)
}

void verify(bool check, error_error_t toSignal, SHORT_STDPARAMS) {
    debug(" :::: Calling extern " T4LIT(verify,extern) "\n");
}

void verify_checksum_with_payload(bool condition, struct uint8_buffer_s data, bitfield_handle_t checksum, enum_PSA_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    debug(" :::: Calling extern " T4LIT(verify_checksum_with_payload,extern) "\n");
}

void update_checksum_with_payload(bool condition, struct uint8_buffer_s data, bitfield_handle_t checksum, enum_PSA_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    debug(" :::: Calling extern " T4LIT(update_checksum_with_payload,extern) "\n");
}
