// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_v1model.h"
#include "util_packet.h"
#include "util_debug.h"

#include "dpdk_lib.h"
#include "stateful_memory.h"

#include "rte_hash_crc.h"

#include <rte_ip.h>

void hash(uint16_t* result, enum_HashAlgorithm_t hash, uint16_t base, uint8_buffer_t data, uint32_t max, SHORT_STDPARAMS) {
    debug("    : Executing hash\n");
}

void hash__u32__u16__bufs__u32(uint16_t* result, enum_HashAlgorithm_t hash, uint16_t base, uint8_buffer_t data, uint32_t max, SHORT_STDPARAMS) {
    dbg_bytes(data.buffer, data.buffer_size, "    : Executing hash on " T4LIT(%d) " bytes: ", data.buffer_size);
    if (hash == enum_HashAlgorithm_crc32){
        uint32_t crc32_result = rte_hash_crc(data.buffer, data.buffer_size, 0xffffffff);
        memcpy(result, &crc32_result, 4);
    } else if (hash == enum_HashAlgorithm_identity) {
        memcpy(result, data.buffer, data.buffer_size > 8 ? 8 : data.buffer_size);
    } else {
        debug("    : Unknown hashing method!\n");
    }
    dbg_bytes(result, 4, "        Result:");
}

void hash__u16__u16__u16s__u32(uint16_t* result, enum_HashAlgorithm_t hash, uint16_t base, uint8_buffer_t data, uint32_t max, SHORT_STDPARAMS) {
    debug("    : Executing hash__u16__u16__u16s__u32\n");
}

void hash__u16__u16__u32s__u32(uint16_t* result, enum_HashAlgorithm_t hash, uint16_t base, uint8_buffer_t data, uint32_t max, SHORT_STDPARAMS) {
    debug("    : Executing hash__u16__u16__u32s__u32\n");
}

void hash__u16__u16__bufs__u32(uint16_t* result, enum_HashAlgorithm_t hash, uint16_t base, uint8_buffer_t data, uint32_t max, SHORT_STDPARAMS) {
    debug("    : Executing hash__u16__u16__ubufs__u32\n");
}
