// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_v1model.h"
#include "util_packet.h"

#include <rte_ip.h>

int egress_port_field() {
    return FLD(all_metadatas,egress_spec);
}

int ingress_port_field() {
    return FLD(all_metadatas,ingress_port);
}

void transfer_to_egress(packet_descriptor_t* pd)
{
    // int res32; // needed for the macro
    // uint32_t val = GET_INT32_AUTO_PACKET(pd, HDR(all_metadatas), egress_port_field());
    // MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), FLD(all_metadatas,egress_spec), val);
}

int extract_egress_port(packet_descriptor_t* pd) {
    return GET_INT32_AUTO_PACKET(pd, HDR(all_metadatas), egress_port_field());
}

int extract_ingress_port(packet_descriptor_t* pd) {
    return GET_INT32_AUTO_PACKET(pd, HDR(all_metadatas), ingress_port_field());
}

void set_handle_packet_metadata(packet_descriptor_t* pd, uint32_t portid)
{
    int res32; // needed for the macro
    MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), ingress_port_field(), portid);
}

void verify_checksum(bool cond, struct uint8_buffer_s data, bitfield_handle_t cksum_field_handle, enum enum_HashAlgorithm algorithm, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(verify_checksum,extern) "\n");
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
            debug("    " T4LIT(!!,error) " Packet checksum is " T4LIT(wrong,error) ": " T4LIT(%04x,bytes) ", calculated checksum is " T4LIT(%04x,bytes) "\n", current_cksum, calculated_cksum);
        }
#endif

        if (unlikely(calculated_cksum != current_cksum)) {
            MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), FLD(all_metadatas,checksum_error), 1)
        }
    }
}

void update_checksum(bool cond, struct uint8_buffer_s data, bitfield_handle_t cksum_field_handle, enum enum_HashAlgorithm algorithm, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(update_checksum,extern) "\n");

    uint32_t res32, calculated_cksum = 0;
    if(cond) {
        if (algorithm == enum_HashAlgorithm_csum16) {
            calculated_cksum = rte_raw_cksum(data.buffer, data.buffer_size);
            calculated_cksum = (calculated_cksum == 0xffff) ? calculated_cksum : ((~calculated_cksum) & 0xffff);
        }

        debug("       : Packet checksum " T4LIT(updated,status) " to " T4LIT(%04x,bytes) "\n", calculated_cksum);

        // TODO temporarily disabled: this line modifies a lookup table's pointer instead of a checksum field
        // MODIFY_INT32_INT32_BITS(cksum_field_handle, calculated_cksum)
    }
}

void verify_checksum_offload(bitfield_handle_t cksum_field_handle, enum enum_HashAlgorithm algorithm, SHORT_STDPARAMS) {
    debug("    : Called extern " T4LIT(verify_checksum_offload,extern) "\n");
    
    if ((pd->wrapper->ol_flags & PKT_RX_IP_CKSUM_BAD) != 0) {
        uint32_t res32;
        MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), FLD(all_metadatas,checksum_error), 1)

        debug("       : Verifying packet checksum: " T4LIT(%04x,bytes) "\n", res32);
    }
}

void update_checksum_offload(bitfield_handle_t cksum_field_handle, enum enum_HashAlgorithm algorithm, uint8_t len_l2, uint8_t len_l3, SHORT_STDPARAMS) {
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
    MODIFY_INT32_INT32_BITS_PACKET(pd, HDR(all_metadatas), FLD(all_metadatas,drop), 1)

    debug("       : " T4LIT(standard_metadata,header) "." T4LIT(drop,field) " = " T4LIT(1,bytes) "\n");
}

void verify(bool check, enum error_error toSignal, SHORT_STDPARAMS) {
    // TODO implement call to extern
    debug("    : Called extern " T4LIT(verify,extern) "\n");
}

void verify_checksum_with_payload(bool condition, struct uint8_buffer_s data, bitfield_handle_t checksum, enum enum_HashAlgorithm algo, SHORT_STDPARAMS) {
    // TODO implement call to extern
    debug("    : Called extern " T4LIT(verify_checksum_with_payload,extern) "\n");
}

void update_checksum_with_payload(bool condition, struct uint8_buffer_s data, bitfield_handle_t checksum, enum enum_HashAlgorithm algo, SHORT_STDPARAMS) {
    // TODO implement call to extern
    debug("    : Called extern " T4LIT(update_checksum_with_payload,extern) "\n");
}
