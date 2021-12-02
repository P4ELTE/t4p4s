// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_v1model.h"
#include "dpdk_model_v1model_funs.h"
#include "util_packet.h"
#include "util_debug.h"

#include "dpdk_lib.h"
#include "stateful_memory.h"

#include <rte_hash_crc.h>
#include <rte_compat.h>

#include <rte_ip.h>


// TODO will this always work for both 32b and 16b hashes?
void EXTERNIMPL0(hash)(uint32_t* result, enum_HashAlgorithm_t hash, uint16_t base, uint8_buffer_t data, uint32_t max, SHORT_STDPARAMS) {
    debug("    : Executing hash on " T4LIT(%d) " using %s\n", data.size, enum_value_names_HashAlgorithm[hash]);
    dbg_bytes(data.buffer, data.size, "    : Executing hash on " T4LIT(%d) " bytes: ", data.size);

    switch (hash) {
        case enum_HashAlgorithm_crc32:
        {
            *result = rte_hash_crc(data.buffer, data.size, 0xffffffff);
        }
            break;

        case enum_HashAlgorithm_identity:
        {
            if (data.size > 4){
                memcpy(result, data.buffer, 4);
            } else {
                *result = 0;
                memcpy(result, data.buffer, data.size);
            }
        }
            break;

        case enum_HashAlgorithm_random:
        {
            *result = rte_rand();
        }
            break;

        default:
        {
            debug("    " T4LIT(Not implemented hash algorithm!,error) " fallback to identity algorithm" "\n");
            memcpy(result, data.buffer, data.size > 4 ? 4 : data.size);
        }
    }

    dbg_bytes(result, 4, "        Result:");
}

void EXTERNIMPL4(hash,u16,u16,u8s,u32)(uint16_t* result, enum_HashAlgorithm_t hash, uint16_t base, uint8_buffer_t data, uint32_t max, SHORT_STDPARAMS) {
    dbg_bytes(data.buffer, data.size, "    : Executing EXTERNIMPL4(hash,u16,u16,u8s,u32) on " T4LIT(%d) " bytes: ", data.size);
    switch(hash) {
        case enum_HashAlgorithm_csum16:
        {
            *result = rte_raw_cksum(data.buffer, data.size);
        }
            break;

        case enum_HashAlgorithm_identity:
        {
            *result = 0;
            memcpy(result, data.buffer, data.size > 2 ? 2 : data.size);
        }
            break;

        case enum_HashAlgorithm_random:
        {
            *result = rte_rand();
        }
            break;

        case enum_HashAlgorithm_xor16:
        {
            *result = 0;
            for (int a = 0; a < data.size; a++){
                *((uint8_t*)result + (a & 1)) ^= data.buffer[a];
            }
        }
            break;

        default:
        {
            debug("    " T4LIT(Not implemented hash algorithm!,error) " fallback to identity algorithm" "\n");
            memcpy(result, data.buffer, data.size > 2 ? 2 : data.size);
        }
    }
    dbg_bytes(result, 2, "        Result:");
}

void EXTERNIMPL4(hash,u32,u16,u8s,u32)(uint32_t* result, enum_HashAlgorithm_t hash, uint16_t base, uint8_buffer_t data, uint32_t max, SHORT_STDPARAMS) {
    dbg_bytes(data.buffer, data.size, "    : Executing EXTERNIMPL4(hash,u32,u16,u8s,u32) on " T4LIT(%d) " bytes: ", data.size);

    switch(hash){
        case enum_HashAlgorithm_crc32:
        {
            *result = rte_hash_crc(data.buffer, data.size, 0xffffffff);
        }
            break;

        case enum_HashAlgorithm_identity:
        {
            if(data.size > 4){
                memcpy(result, data.buffer, 4);
            } else {
                *result = 0;
                memcpy(result, data.buffer, data.size);
            }
        }
            break;

        case enum_HashAlgorithm_random:
        {
            *result = rte_rand();
        }
            break;

        default:
        {
            debug("    " T4LIT(Not implemented hash algorithm!,error) " fallback to identity algorithm" "\n");
            memcpy(result, data.buffer, data.size > 4 ? 4 : data.size);
        }
    }

    dbg_bytes(result, 4, "        Result:");
}


void hash(uint16_t* result, enum_HashAlgorithm_t hash, uint16_t base, uint8_buffer_t data, uint32_t max, SHORT_STDPARAMS) {
    EXTERNIMPL4(hash,u16,u16,u8s,u32)(result,hash,base,data,max, SHORT_STDPARAMS_IN);
}

void hash__u16__u16__u16s__u32(uint16_t* result, enum_HashAlgorithm_t hash, uint16_t base, uint8_buffer_t data, uint32_t max, SHORT_STDPARAMS) {
    EXTERNIMPL4(hash,u16,u16,u8s,u32)(result,hash,base,data,max, SHORT_STDPARAMS_IN);
}

void hash__u16__u16__u32s__u32(uint16_t* result, enum_HashAlgorithm_t hash, uint16_t base, uint8_buffer_t data, uint32_t max, SHORT_STDPARAMS) {
    EXTERNIMPL4(hash,u16,u16,u8s,u32)(result,hash,base,data,max, SHORT_STDPARAMS_IN);
}


void hash__u32__u16__u16s__u32(uint32_t* result, enum_HashAlgorithm_t hash, uint16_t base, uint8_buffer_t data, uint32_t max, SHORT_STDPARAMS) {
    EXTERNIMPL4(hash,u32,u16,u8s,u32)(result,hash,base,data,max, SHORT_STDPARAMS_IN);
}

void hash__u32__u16__u32s__u32(uint32_t* result, enum_HashAlgorithm_t hash, uint16_t base, uint8_buffer_t data, uint32_t max, SHORT_STDPARAMS) {
    EXTERNIMPL4(hash,u32,u16,u8s,u32)(result,hash,base,data,max, SHORT_STDPARAMS_IN);
}

void EXTERNIMPL1(hash,u8s)(uint32_t* /* out */ result, enum_HashAlgorithm_t hash, uint16_t base, uint8_buffer_t data, uint32_t max, SHORT_STDPARAMS) {
    EXTERNIMPL4(hash,u32,u16,u8s,u32)(result,hash,base,data,max, SHORT_STDPARAMS_IN);
}
