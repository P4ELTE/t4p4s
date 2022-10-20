// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "backend.h"

#include "util_packet.h"
#include "common.h"

#include "dpdk_smem_repr.h"

typedef enum_CounterType_t T4P4S_COUNTER_e;
typedef enum_MeterType_t T4P4S_METER_e;

#include "dpdk_model_v1model_funs.h"


#define T4P4S_MODEL v1model

#define INGRESS_META_FLD    FLD(all_metadatas,ingress_port)
#define EGRESS_META_FLD     FLD(all_metadatas,egress_port)
#define EGRESS_INIT_VALUE   0
// note: EGRESS_DROP_VALUE should not clash with T4P4S_BROADCAST_PORT
#define EGRESS_DROP_VALUE   200


void set_handle_packet_metadata(packet_descriptor_t* pd, uint32_t portid);


void SHORT_EXTERNCALL1(counter,count)(uint32_t counter_array_size, T4P4S_COUNTER_e ct, uint32_t index, SMEMTYPE(counter)* counter, SHORT_STDPARAMS);
void SHORT_EXTERNCALL1(direct_counter,count)(int counter_type, SMEMTYPE(direct_counter)* smem, SHORT_STDPARAMS);
void EXTERNCALL1(meter,execute_meter,u32)(uint32_t index, T4P4S_METER_e b, uint32_t c, uint32_t* d, SMEMTYPE(meter)* e, SHORT_STDPARAMS);
void EXTERNCALL1(meter,execute_meter,u16)(uint32_t index, T4P4S_METER_e b, uint16_t c, uint16_t* d, SMEMTYPE(meter)* e, SHORT_STDPARAMS);
void EXTERNCALL1(meter,execute_meter,u8)(uint32_t index, T4P4S_METER_e b, uint8_t c, uint8_t* d, SMEMTYPE(meter)* e, SHORT_STDPARAMS);
void EXTERNCALL1(direct_meter,read,u8)(T4P4S_METER_e b, uint8_t* result, SMEMTYPE(direct_meter)* e, SHORT_STDPARAMS);
void EXTERNCALL1(direct_meter,read,u16)(T4P4S_METER_e b, uint16_t* result, SMEMTYPE(direct_meter)* e, SHORT_STDPARAMS);
void EXTERNCALL1(direct_meter,read,u32)(T4P4S_METER_e b, uint32_t* result, SMEMTYPE(direct_meter)* e, SHORT_STDPARAMS);

void SHORT_EXTERNCALL1(init_register,i8)(register_int8_t* reg, int8_t size, SHORT_STDPARAMS);
void SHORT_EXTERNCALL1(init_register,i16)(register_int16_t* reg, int16_t size, SHORT_STDPARAMS);
void SHORT_EXTERNCALL1(init_register,i32)(register_int32_t* reg, int32_t size, SHORT_STDPARAMS);
void SHORT_EXTERNCALL1(init_register,i64)(register_int64_t* reg, int64_t size, SHORT_STDPARAMS);
void SHORT_EXTERNCALL1(init_register,u8)(register_uint8_t* reg, uint8_t size, SHORT_STDPARAMS);
void SHORT_EXTERNCALL1(init_register,u16)(register_uint16_t* reg, uint16_t size, SHORT_STDPARAMS);
void SHORT_EXTERNCALL1(init_register,u32)(register_uint32_t* reg, uint32_t size, SHORT_STDPARAMS);
void SHORT_EXTERNCALL1(init_register,u64)(register_uint64_t* reg, uint64_t size, SHORT_STDPARAMS);
void EXTERNCALL1(register,read,i8)(int8_t x, int8_t* value_result, int8_t idx, register_int8_t* reg, SHORT_STDPARAMS);
void EXTERNCALL1(register,read,i16)(int16_t x, int16_t* value_result, int16_t idx, register_int16_t* reg, SHORT_STDPARAMS);
void EXTERNCALL1(register,read,i32)(int32_t x, int32_t* value_result, int32_t idx, register_int32_t* reg, SHORT_STDPARAMS);
void EXTERNCALL1(register,read,i64)(int64_t x, int64_t* value_result, int64_t idx, register_int64_t* reg, SHORT_STDPARAMS);
void EXTERNCALL1(register,read,u8)(uint8_t x, uint8_t* value_result, uint8_t idx, register_uint8_t* reg, SHORT_STDPARAMS);
void EXTERNCALL1(register,read,u16)(uint16_t x, uint16_t* value_result, uint16_t idx, register_uint16_t* reg, SHORT_STDPARAMS);
void EXTERNCALL1(register,read,u32)(uint32_t x, uint32_t* value_result, uint32_t idx, register_uint32_t* reg, SHORT_STDPARAMS);
void EXTERNCALL1(register,read,u64)(uint64_t x, uint64_t* value_result, uint64_t idx, register_uint64_t* reg, SHORT_STDPARAMS);
void EXTERNCALL1(register,write,i8)(int8_t x, int idx, int8_t value, register_int8_t* reg, SHORT_STDPARAMS);
void EXTERNCALL1(register,write,i16)(int16_t x, int idx, int16_t value, register_int16_t* reg, SHORT_STDPARAMS);
void EXTERNCALL1(register,write,i32)(int32_t x, int idx, int32_t value, register_int32_t* reg, SHORT_STDPARAMS);
void EXTERNCALL1(register,write,i64)(int64_t x, int idx, int64_t value, register_int64_t* reg, SHORT_STDPARAMS);
void EXTERNCALL1(register,write,u8)(uint8_t x, int idx, uint8_t value, register_uint8_t* reg, SHORT_STDPARAMS);
void EXTERNCALL1(register,write,u16)(uint16_t x, int idx, uint16_t value, register_uint16_t* reg, SHORT_STDPARAMS);
void EXTERNCALL1(register,write,u32)(uint32_t x, int idx, uint32_t value, register_uint32_t* reg, SHORT_STDPARAMS);
void EXTERNCALL1(register,write,u64)(uint64_t x, int idx, uint64_t value, register_uint64_t* reg, SHORT_STDPARAMS);
