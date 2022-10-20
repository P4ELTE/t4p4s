// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include <stdbool.h>
#include "backend.h"
#include "dpdk_smem.h"

#include "util_packet.h"
#include "common.h"

#define T4P4S_MODEL psa

#define INGRESS_META_FLD    FLD(all_metadatas,ingress_port)
#define EGRESS_META_FLD     FLD(all_metadatas,egress_port)
#define EGRESS_INIT_VALUE   0
// note: EGRESS_DROP_VALUE should not clash with T4P4S_BROADCAST_PORT
#define EGRESS_DROP_VALUE   200


typedef enum_PSA_CounterType_t T4P4S_COUNTER_e;
typedef enum_PSA_MeterType_t T4P4S_METER_e;


void set_handle_packet_metadata(packet_descriptor_t* pd, uint32_t portid);

// p4.psa: bool psa_normal(in psa_ingress_output_metadata_t istd);
// p4.psa: bool psa_clone_i2e(in psa_ingress_output_metadata_t istd);
// p4.psa: bool psa_clone_e2e(in psa_egress_output_metadata_t istd);
// p4.psa: bool psa_resubmit(in psa_ingress_output_metadata_t istd);
// p4.psa: bool psa_recirculate(in psa_egress_output_metadata_t istd,

// p4.psa: void assert(in bool check);
// p4.psa: void assume(in bool check);

// p4.psa: PacketReplicationEngine {...}
// p4.psa: BufferingQueueingEngine {...}

// p4.psa: ActionProfile {...}
// p4.psa: ActionSelector {...}

// p4.psa: Hash<O> {...}

// p4.psa: Checksum<W> {...}
#ifdef externdef_Checksum_u32
    void EXTERNIMPL3(Checksum,update,u8s,u32)(EXTERNTYPE1(Checksum,u32)* checksum, enum_PSA_HashAlgorithm_t algo, uint8_buffer_t data, SHORT_STDPARAMS);

    void EXTERNIMPL2(verify_checksum_offload,u8s,u16)(uint8_buffer_t data, SHORT_STDPARAMS);
#endif


void verify(bool check, error_error_t toSignal, SHORT_STDPARAMS);
void verify_checksum__b8s__b16(bool cond, uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum_PSA_HashAlgorithm_t algorithm, SHORT_STDPARAMS);
void verify_checksum_offload(bitfield_handle_t cksum_field_handle, enum_PSA_HashAlgorithm_t algorithm, SHORT_STDPARAMS);
void verify_checksum_with_payload(bool condition, uint8_buffer_t data, bitfield_handle_t checksum, enum_PSA_HashAlgorithm_t algo, SHORT_STDPARAMS);
void update_checksum__b8s__b16(bool cond, uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum_PSA_HashAlgorithm_t algorithm, SHORT_STDPARAMS);
void update_checksum_offload(bitfield_handle_t cksum_field_handle, enum_PSA_HashAlgorithm_t algorithm, uint8_t len_l2, uint8_t len_l3, SHORT_STDPARAMS);
void update_checksum_with_payload(bool condition, uint8_buffer_t data, bitfield_handle_t checksum, enum_PSA_HashAlgorithm_t algo, SHORT_STDPARAMS);

// p4.psa: Random<T> {...}

// p4.psa: Digest<T> {...}

// p4.psa: Counter<W, S> {...}
// void EXTERNCALL0(Counter,count)(SMEMTYPE(counter)* counter, int index, uint32_t value);
void EXTERNCALL2(Counter,count,u32,u8)();
void EXTERNCALL2(Counter,count,u32,u32)();
void EXTERNCALL2(Counter,count,i32,i32)();
void EXTERNCALL2(Counter,count,buf,u8)();

// p4.psa: DirectCounter<W> {...}

// p4.psa: Meter<S> {...}
// enum_PSA_MeterColor_t EXTERNCALL0(Meter,execute)(SMEMTYPE(meter)* smem, int x, SHORT_STDPARAMS);
// enum_PSA_MeterColor_t EXTERNCALL1(Meter,execute,i32)(uint32_t x, enum_PSA_MeterType_t meter_type, int index, int color, SMEMTYPE(meter)* smem, SHORT_STDPARAMS);

// p4.psa: DirectMeter {...}
// p4.psa: Register<T, S> {...}
    // void extern_counter_count(uint32_t counter_array_size, T4P4S_COUNTER_e ct, uint32_t index, counter_t* counter, SHORT_STDPARAMS);
    // void extern_meter_execute_meter(uint32_t index, T4P4S_METER_e b, uint32_t c, uint8_t d, meter_t e, SHORT_STDPARAMS);
    // void extern_register_read(uint32_t index, uint32_t a, uint32_t b, register_t c, SHORT_STDPARAMS);
    // void extern_register_write(uint32_t index, uint32_t a, uint32_t b, register_t* c, SHORT_STDPARAMS);

// p4.psa: InternetChecksum {...}
// typedef uint16_t InternetChecksum;

// void EXTERNCALL0(InternetChecksum,init)(EXTERNTYPE0(InternetChecksum)* checksum, SHORT_STDPARAMS);
// void EXTERNCALL0(InternetChecksum,clear)(EXTERNTYPE0(InternetChecksum)* checksum, SHORT_STDPARAMS);
// void EXTERNCALL0(InternetChecksum,add)(EXTERNTYPE0(InternetChecksum)* checksum, SHORT_STDPARAMS);
// uint16_t EXTERNCALL0(InternetChecksum,get)(EXTERNTYPE0(InternetChecksum)* checksum, SHORT_STDPARAMS);
// void EXTERNCALL0(InternetChecksum,subtract)(EXTERNTYPE0(InternetChecksum)* checksum, uint16_t data, SHORT_STDPARAMS);
// uint16_t EXTERNCALL0(InternetChecksum,get_state)(EXTERNTYPE0(InternetChecksum)* checksum, SHORT_STDPARAMS);
// void EXTERNCALL0(InternetChecksum,set_state)(EXTERNTYPE0(InternetChecksum)* checksum, uint16_t checksum_state, SHORT_STDPARAMS);



// void EXTERNCALL0(Counter,count)(uint32_t counter_array_size, T4P4S_COUNTER_e ct, uint32_t index, SMEMTYPE(counter)* counter, SHORT_STDPARAMS);
// void EXTERNCALL0(Direct_counter,count)(int counter_type, SMEMTYPE(direct_counter)* smem, SHORT_STDPARAMS);
// void EXTERNCALL1(Meter,execute_meter,u32)(uint32_t index, T4P4S_METER_e b, uint32_t c, uint8_t* d, SMEMTYPE(meter)* e, SHORT_STDPARAMS);
// void EXTERNCALL1(Direct_meter,read,u8)(T4P4S_METER_e b, uint32_t c, SMEMTYPE(meter)* e, SHORT_STDPARAMS);
// void EXTERNCALL0(Register,read)(uint32_t index, uint32_t a, uint32_t b, REGTYPE(uint,32) c, SHORT_STDPARAMS);
// void EXTERNCALL2(Register,write,u32,u32)(uint32_t index, uint32_t a, uint32_t b, REGTYPE(uint,32)* c, SHORT_STDPARAMS);
