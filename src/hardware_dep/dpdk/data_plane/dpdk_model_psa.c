// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_psa.h"

#include <rte_ip.h>
#include "actions.h"
#include "util_debug.h"

#include "dpdk_lib.h"
#include "stateful_memory.h"

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

void mark_to_drop(SHORT_STDPARAMS) {
    debug(" :::: Calling extern " T4LIT(mark_to_drop,extern) "\n");

    uint32_t res32;
    MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), EGRESS_META_FLD, EGRESS_DROP_VALUE)
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


extern void do_counter_count(counter_t* counter, int index, uint32_t value);

void extern_counter_count(uint32_t counter_array_size, enum_PSA_CounterType_t ct, uint32_t index, counter_t* counter, SHORT_STDPARAMS) {
    do_counter_count(counter, index, ct == enum_PSA_CounterType_t_PACKETS ? 1 : packet_length(pd));
}

void extern_meter_execute_meter(uint32_t index, enum_PSA_MeterType_t b, uint32_t c, uint8_t d, meter_t e, SHORT_STDPARAMS) {
    debug("    : Executing extern_meter_execute_meter#" T4LIT(%d) "\n", index);
}

void extern_register_read(uint32_t index, uint32_t a, uint32_t b, register_t c, SHORT_STDPARAMS) {
    debug("    : Executing extern_register_read#" T4LIT(%d) "\n", index);
}

void extern_register_write(uint32_t index, uint32_t a, uint32_t b, register_t* c, SHORT_STDPARAMS) {
    debug("    : Executing extern_register_write#" T4LIT(%d) "\n", index);
}
