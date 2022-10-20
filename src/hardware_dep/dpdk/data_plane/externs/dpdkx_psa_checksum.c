// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_psa.h"

#include <rte_ip.h>
#include "actions.h"
#include "util_debug.h"

#include "dpdk_lib.h"
#include "stateful_memory.h"

extern struct all_metadatas_t all_metadatas;

void EXTERNIMPL0(verify)(bool check, error_error_t toSignal, SHORT_STDPARAMS) {
    // TODO implement call to extern
    debug("    : Called extern " T4LIT(verify,extern) "\n");
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

//void verify(bool check, error_error_t toSignal, SHORT_STDPARAMS) {
//    debug(" :::: Calling extern " T4LIT(verify,extern) "\n");
//}

void verify_checksum_with_payload(bool condition, uint8_buffer_t data, bitfield_handle_t checksum, enum_PSA_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    debug(" :::: Calling extern " T4LIT(verify_checksum_with_payload,extern) "\n");
}

void update_checksum_with_payload(bool condition, uint8_buffer_t data, bitfield_handle_t checksum, enum_PSA_HashAlgorithm_t algo, SHORT_STDPARAMS) {
    debug(" :::: Calling extern " T4LIT(update_checksum_with_payload,extern) "\n");
}

////////////////////////////////

const char*const names_PSA_HashAlgorithm_t[] = {
    "IDENTITY",
    "CRC32",
    "CRC32_CUSTOM",
    "CRC16",
    "CRC16_CUSTOM",
    "ONES_COMPLEMENT16",
    "TARGET_DEFAULT",
};

#ifdef externdef_Checksum_u32
    void EXTERNIMPL3(Checksum,update,u8s,u32)(EXTERNTYPE1(Checksum,u32)* checksum, enum_PSA_HashAlgorithm_t algo, uint8_buffer_t data,  SHORT_STDPARAMS) {
        const char*const algo_name = names_PSA_HashAlgorithm_t[algo];
        debug(" :::: calling extern impl " T4LIT(Checksum/update/u32,extern) " with algorithm " T4LIT(%s) "\n", algo_name);
    }

    void EXTERNIMPL2(verify_checksum_offload,u8s,u16)(uint8_buffer_t data, SHORT_STDPARAMS) {
        debug(" :::: calling extern impl " T4LIT(Checksum/verify_checksum_offload/u8s/u16,extern) "\n");
    }
#endif

////////////////////////////////

void EXTERNIMPL1(InternetChecksum,init)(EXTERNTYPE0(InternetChecksum)* checksum, SHORT_STDPARAMS) {
    debug(" :::: calling extern " T4LIT(InternetChecksum_t_init,extern) "\n");
}

void EXTERNIMPL1(InternetChecksum,clear)(EXTERNTYPE0(InternetChecksum)* checksum, SHORT_STDPARAMS) {
    debug(" :::: calling extern " T4LIT(extern_InternetChecksum_clear,extern) "\n");
}

void EXTERNIMPL2(InternetChecksum,add,u8s)(EXTERNTYPE0(InternetChecksum)* checksum, uint8_buffer_t data, SHORT_STDPARAMS) {
    debug(" :::: calling extern " T4LIT(extern_InternetChecksum_add,extern) "\n");
}

uint16_t EXTERNIMPL1(InternetChecksum,get)(EXTERNTYPE0(InternetChecksum)* checksum, SHORT_STDPARAMS) {
    debug(" :::: calling extern " T4LIT(InternetChecksum_t_get,extern) "\n");
    return 0;
}

void EXTERNIMPL1(InternetChecksum,subtract)(EXTERNTYPE0(InternetChecksum)* checksum, uint16_t data, SHORT_STDPARAMS) {
    debug(" :::: calling extern " T4LIT(InternetChecksum_t_subtract,extern) "\n");
}

uint16_t EXTERNIMPL1(InternetChecksum,get_state)(EXTERNTYPE0(InternetChecksum)* checksum, SHORT_STDPARAMS) {
    debug(" :::: calling extern " T4LIT(InternetChecksum_t_get_state,extern) "\n");
    return 0;
}

void EXTERNIMPL1(InternetChecksum,set_state)(EXTERNTYPE0(InternetChecksum)* checksum, uint16_t checksum_state, SHORT_STDPARAMS) {
    debug(" :::: calling extern " T4LIT(InternetChecksum_t_set_state,extern) "\n");
}


enum_PSA_MeterColor_t EXTERNCALL0(Meter,execute)(SMEMTYPE(meter)* smem, int x, SHORT_STDPARAMS) {
    debug(" :::: calling extern " T4LIT(Meter_execute,extern) "\n");
    // TODO temporary implementation
    return enum_PSA_MeterColor_t_RED;
}

enum_PSA_MeterColor_t EXTERNIMPL2(Meter,execute,i32)(uint32_t x, enum_PSA_MeterType_t meter_type, int index, int color, SMEMTYPE(meter)* smem, SHORT_STDPARAMS) {
    debug(" :::: calling extern " T4LIT(Meter_execute,extern) "\n");
    // TODO temporary implementation
    return enum_PSA_MeterColor_t_RED;
}
