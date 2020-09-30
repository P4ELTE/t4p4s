// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include <stdbool.h>
#include "backend.h"

#include "util_packet.h"
#include "common.h"

#define INGRESS_META_FLD    FLD(all_metadatas,ingress_port)
#define EGRESS_META_FLD     FLD(all_metadatas,egress_port)
#define EGRESS_INIT_VALUE   0
#define EGRESS_DROP_VALUE   true

void verify_checksum(bool cond, uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum_PSA_HashAlgorithm_t algorithm, SHORT_STDPARAMS);
void update_checksum(bool cond, uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum_PSA_HashAlgorithm_t algorithm, SHORT_STDPARAMS);
void verify_checksum_offload(bitfield_handle_t cksum_field_handle, enum_PSA_HashAlgorithm_t algorithm, SHORT_STDPARAMS);
void update_checksum_offload(bitfield_handle_t cksum_field_handle, enum_PSA_HashAlgorithm_t algorithm, uint8_t len_l2, uint8_t len_l3, SHORT_STDPARAMS);

void mark_to_drop(SHORT_STDPARAMS);

void set_handle_packet_metadata(packet_descriptor_t* pd, uint32_t portid);
