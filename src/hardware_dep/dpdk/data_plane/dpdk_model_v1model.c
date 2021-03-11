// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_v1model.h"
#include "util_packet.h"
#include "util_debug.h"

#include "dpdk_lib.h"
#include "stateful_memory.h"

#include <rte_ip.h>

void transfer_to_egress(packet_descriptor_t* pd)
{
    // int res32; // needed for the macro
    // uint32_t val = GET_INT32_AUTO_PACKET(pd, HDR(all_metadatas), EGRESS_META_FLD);
    // MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), EGRESS_META_FLD, val);
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

void verify_checksum__b8s__b16(bool cond, uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS) {
    dbg_bytes(data.buffer, data.buffer_size, "    : " T4LIT(Verifying checksum,extern) " for " T4LIT(%d) " bytes: ", data.buffer_size);
    uint32_t res32, current_cksum = 0, calculated_cksum = 0;
    if (cond) {
        if (algorithm == enum_HashAlgorithm_csum16) {
            calculated_cksum = rte_raw_cksum(data.buffer, data.buffer_size);
            calculated_cksum = (calculated_cksum == 0xffff) ? calculated_cksum : ((~calculated_cksum) & 0xffff);
            EXTRACT_INT32_BITS(cksum_field_handle, current_cksum)
        }

#ifdef T4P4S_DEBUG
        if (current_cksum == calculated_cksum) {
            debug("      : Packet checksum is " T4LIT(ok,success) ": " T4LIT(%04x,bytes) "\n", current_cksum);
        } else {
            debug("    " T4LIT(!,error) " Packet checksum is " T4LIT(wrong,error) ": " T4LIT(%04x,bytes) ", calculated checksum is " T4LIT(%04x,bytes) "\n", current_cksum, calculated_cksum);
        }
#endif

        if (unlikely(calculated_cksum != current_cksum)) {
            MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), FLD(all_metadatas,checksum_error), 1)
        }
    }
}

void update_checksum__b8s__b16(bool cond, uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS) {
    dbg_bytes(data.buffer, data.buffer_size, "    : " T4LIT(Updating checksum,extern) " for " T4LIT(%d) " bytes: ", data.buffer_size);

    uint32_t res32, calculated_cksum = 0;
    if (cond) {
        if (algorithm == enum_HashAlgorithm_csum16) {
            calculated_cksum = rte_raw_cksum(data.buffer, data.buffer_size);
            calculated_cksum = (calculated_cksum == 0xffff) ? calculated_cksum : ((~calculated_cksum) & 0xffff);
        }

        debug("       : Packet checksum " T4LIT(updated,status) " to " T4LIT(%04x,bytes) "\n", calculated_cksum);

        // TODO temporarily disabled: this line modifies a lookup table's pointer instead of a checksum field
        MODIFY_INT32_INT32_BITS(cksum_field_handle, calculated_cksum)
    }
}

void verify_checksum_offload__b8s__b16(bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(verify_checksum_offload,extern) "\n");

    if ((pd->wrapper->ol_flags & PKT_RX_IP_CKSUM_BAD) != 0) {
        uint32_t res32;
        MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), FLD(all_metadatas,checksum_error), 1)

        debug("       : Verifying packet checksum: " T4LIT(%04x,bytes) "\n", res32);
    }
}

void verify_checksum_offload(bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(verify_checksum_offload,extern) "\n");

    if ((pd->wrapper->ol_flags & PKT_RX_IP_CKSUM_BAD) != 0) {
        uint32_t res32;
        MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), FLD(all_metadatas,checksum_error), 1)

        debug("       : Verifying packet checksum: " T4LIT(%04x,bytes) "\n", res32);
    }
}

void update_checksum_offload(bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, uint8_t len_l2, uint8_t len_l3, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(update_checksum_offload,extern) "\n");

    pd->wrapper->l2_len = len_l2;
    pd->wrapper->l3_len = len_l3;
    pd->wrapper->ol_flags |= PKT_TX_IPV4 | PKT_TX_IP_CKSUM;
    uint32_t res32;
    MODIFY_INT32_INT32_BITS(cksum_field_handle, 0)

    debug("       : Updating packet checksum (offload)\n");
    // TODO implement offload
}

void mark_to_drop(SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(mark_to_drop,extern) "\n");

    uint32_t res32;
    MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), EGRESS_META_FLD, EGRESS_DROP_VALUE)

    debug("       : " T4LIT(all_metadatas,header) "." T4LIT(EGRESS_META_FLD,field) " = " T4LIT(EGRESS_DROP_VALUE,bytes) "\n");
}

void verify(bool check, error_error_t toSignal, SHORT_STDPARAMS) {
    // TODO implement call to extern
    debug("    : Called extern " T4LIT(verify,extern) "\n");
}

void verify_checksum_with_payload(bool condition, uint8_buffer_t data, bitfield_handle_t checksum, enum_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    // TODO implement call to extern
    debug("    : Called extern " T4LIT(verify_checksum_with_payload,extern) "\n");
}

void update_checksum_with_payload(bool condition, uint8_buffer_t data, bitfield_handle_t checksum, enum_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    // TODO implement call to extern
    debug("    : Called extern " T4LIT(update_checksum_with_payload,extern) "\n");
}

// ----------------------------------------------------------------

void verify_checksum__b4s__b16(bool cond, uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS) {
    verify_checksum__b8s__b16(cond, data, cksum_field_handle, algorithm, SHORT_STDPARAMS_IN);
}

void verify_checksum__b32s__b16(bool cond, uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS) {
    verify_checksum__b8s__b16(cond, data, cksum_field_handle, algorithm, SHORT_STDPARAMS_IN);
}

void update_checksum__b4s__b16(bool cond, uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS) {
    update_checksum__b8s__b16(cond, data, cksum_field_handle, algorithm, SHORT_STDPARAMS_IN);
}

void update_checksum__b32s__b16(bool cond, uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS) {
    update_checksum__b8s__b16(cond, data, cksum_field_handle, algorithm, SHORT_STDPARAMS_IN);
}

void verify_checksum_with_payload__b32s__b16(bool condition, uint8_buffer_t data, bitfield_handle_t checksum, enum_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    verify_checksum_with_payload(condition, data, checksum, algo, SHORT_STDPARAMS_IN);
}

void update_checksum_with_payload__b32s__b16(bool condition, uint8_buffer_t data, bitfield_handle_t checksum, enum_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    update_checksum_with_payload(condition, data, checksum, algo, SHORT_STDPARAMS_IN);
}


// ----------------------------------------------------------------

extern void do_counter_count(counter_t* counter, int index, uint32_t value);

void extern_counter_count(uint32_t counter_array_size, enum_CounterType_t ct, uint32_t index, counter_t* counter, SHORT_STDPARAMS) {
    index = rte_be_to_cpu_32(index);
    if (index < counter_array_size) {
	    do_counter_count(counter, index, ct == enum_CounterType_packets ? 1 : packet_length(pd));
    }
}

void extern_meter_execute_meter(uint32_t index, enum_MeterType_t b, uint32_t c, uint8_t d, meter_t e, SHORT_STDPARAMS) {
    debug("    : Executing extern_meter_execute_meter#" T4LIT(%d) "\n", index);
}

extern void extern_register_read_uint32_t(register_uint32_t* reg, uint32_t* value_result, uint32_t idx);
extern void extern_register_write_uint32_t(register_uint32_t* reg, int idx, uint32_t value);

void extern_register_read(uint32_t size, uint32_t* value_result, uint32_t idx, register_uint32_t* reg, SHORT_STDPARAMS) {
    extern_register_read_uint32_t(reg, value_result, idx);
}

void extern_register_write(uint32_t size, uint32_t idx, uint32_t value, register_uint32_t* reg, SHORT_STDPARAMS) {
    extern_register_write_uint32_t(reg, idx, value);
}
