// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_psa.h"

#include <rte_ip.h>
#include "actions.h"
#include "util_debug.h"

#include "dpdk_lib.h"
#include "stateful_memory.h"

extern struct all_metadatas_t all_metadatas;

void verify_checksum__b8s__b16(bool cond, uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum_PSA_HashAlgorithm_t algorithm, SHORT_STDPARAMS) {
    debug(" :::: Calling extern " T4LIT(verify_checksum,extern) "\n");
}

void update_checksum__b8s__b16(bool cond, uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum_PSA_HashAlgorithm_t algorithm, SHORT_STDPARAMS) {
    debug(" :::: Calling extern " T4LIT(update_checksum,extern) "\n");
}

void verify_checksum_offload(bitfield_handle_t cksum_field_handle, enum_PSA_HashAlgorithm_t algorithm, SHORT_STDPARAMS) {
    debug(" :::: Calling extern " T4LIT(verify_checksum_offload,extern) "\n");
}

void update_checksum_offload(bitfield_handle_t cksum_field_handle, enum_PSA_HashAlgorithm_t algorithm, uint8_t len_l2, uint8_t len_l3, SHORT_STDPARAMS) {
    debug(" :::: Calling extern " T4LIT(update_checksum_offload,extern) "\n");
}

void verify(bool check, error_error_t toSignal, SHORT_STDPARAMS) {
    debug(" :::: Calling extern " T4LIT(verify,extern) "\n");
}

void verify_checksum_with_payload(bool condition, uint8_buffer_t data, bitfield_handle_t checksum, enum_PSA_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    debug(" :::: Calling extern " T4LIT(verify_checksum_with_payload,extern) "\n");
}

void update_checksum_with_payload(bool condition, uint8_buffer_t data, bitfield_handle_t checksum, enum_PSA_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    debug(" :::: Calling extern " T4LIT(update_checksum_with_payload,extern) "\n");
}
