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

#ifndef __DPDK_V1MODEL_EXTERN_H_
#define __DPDK_V1MODEL_EXTERN_H_

#include <stdbool.h>
#include "backend.h"

void verify_checksum(bool cond, struct uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum enum_HashAlgorithm algorithm,
                     packet_descriptor_t* pd, lookup_table_t** tables);

void update_checksum(bool cond, struct uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum enum_HashAlgorithm algorithm,
                     packet_descriptor_t* pd, lookup_table_t** tables);

void verify_checksum_offload(bitfield_handle_t cksum_field_handle, enum enum_HashAlgorithm algorithm,
                     packet_descriptor_t* pd, lookup_table_t** tables);

void update_checksum_offload(bitfield_handle_t cksum_field_handle, enum enum_HashAlgorithm algorithm, uint8_t len_l2, uint8_t len_l3,
                     packet_descriptor_t* pd, lookup_table_t** tables);

void mark_to_drop(packet_descriptor_t* pd, lookup_table_t** tables);

#endif // __DPDK_V1MODEL_EXTERN_H_
