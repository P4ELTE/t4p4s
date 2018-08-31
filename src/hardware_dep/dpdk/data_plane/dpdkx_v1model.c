// Copyright 2017 Eotvos Lorand University, Budapest, Hungary
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "dpdkx_v1model.h"

#include <rte_ip.h>

void verify_checksum(bool cond, struct uint8_buffer_s data, bitfield_handle_t cksum_field_handle, enum enum_HashAlgorithm algorithm,
                     packet_descriptor_t* pd, lookup_table_t** tables) {
    uint32_t res32, current_cksum = 0, calculated_cksum = 0;
    if (cond) {
        if (algorithm == enum_HashAlgorithm_csum16) {
            calculated_cksum = rte_raw_cksum(data.buffer, data.buffer_size);
            calculated_cksum = (calculated_cksum == 0xffff) ? calculated_cksum : ((~calculated_cksum) & 0xffff);
            EXTRACT_INT32_BITS(cksum_field_handle, current_cksum)
        }

        if(calculated_cksum != current_cksum) {
            MODIFY_INT32_INT32_BITS_PACKET(pd, header_instance_standard_metadata, field_standard_metadata_t_checksum_error, 1)
        }
    }
}

void update_checksum(bool cond, struct uint8_buffer_s data, bitfield_handle_t cksum_field_handle, enum enum_HashAlgorithm algorithm,
                     packet_descriptor_t* pd, lookup_table_t** tables) {
    uint32_t res32, calculated_cksum = 0;
    if(cond) {
        if (algorithm == enum_HashAlgorithm_csum16) {
            calculated_cksum = rte_raw_cksum(data.buffer, data.buffer_size);
            calculated_cksum = (calculated_cksum == 0xffff) ? calculated_cksum : ((~calculated_cksum) & 0xffff);
        }

        MODIFY_INT32_INT32_BITS(cksum_field_handle, calculated_cksum)
    }
}

void verify_checksum_offload(bitfield_handle_t cksum_field_handle, enum enum_HashAlgorithm algorithm,
                     packet_descriptor_t* pd, lookup_table_t** tables) {
    
   if((pd->wrapper->ol_flags & PKT_RX_IP_CKSUM_BAD) != 0) {
            uint32_t res32;
            MODIFY_INT32_INT32_BITS_PACKET(pd, header_instance_standard_metadata, field_standard_metadata_t_checksum_error, 1)
        }
}

void update_checksum_offload(bitfield_handle_t cksum_field_handle, enum enum_HashAlgorithm algorithm, uint8_t len_l2, uint8_t len_l3,
                     packet_descriptor_t* pd, lookup_table_t** tables) {
    pd->wrapper->l2_len = len_l2;
    pd->wrapper->l3_len = len_l3;
    pd->wrapper->ol_flags |= PKT_TX_IPV4 | PKT_TX_IP_CKSUM;
    uint32_t res32;
    MODIFY_INT32_INT32_BITS(cksum_field_handle, 0)
}

void mark_to_drop(packet_descriptor_t* pd, lookup_table_t** tables) {
    uint32_t res32;
    MODIFY_INT32_INT32_BITS_PACKET(pd, header_instance_standard_metadata, field_standard_metadata_t_drop, 1)
}
