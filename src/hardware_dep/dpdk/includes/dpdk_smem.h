// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include "util_packet.h"
#include "dataplane_lookup.h"

#include "dpdk_smem_repr.h"
#include "gen_model.h"

//=============================================================================
// Registers

void extern_register_read__i8(int8_t x, int8_t* value_result, int8_t idx, register_int8_t* reg, SHORT_STDPARAMS);
void extern_register_read__i16(int16_t x, int16_t* value_result, int16_t idx, register_int16_t* reg, SHORT_STDPARAMS);
void extern_register_read__i32(int32_t x, int32_t* value_result, int32_t idx, register_int32_t* reg, SHORT_STDPARAMS);
void extern_register_read__i64(int64_t x, int64_t* value_result, int64_t idx, register_int64_t* reg, SHORT_STDPARAMS);
void extern_register_read__u8(uint8_t x, uint8_t* value_result, uint8_t idx, register_uint8_t* reg, SHORT_STDPARAMS);
void extern_register_read__u16(uint16_t x, uint16_t* value_result, uint16_t idx, register_uint16_t* reg, SHORT_STDPARAMS);
void extern_register_read__u32(uint32_t x, uint32_t* value_result, uint32_t idx, register_uint32_t* reg, SHORT_STDPARAMS);
void extern_register_read__u64(uint64_t x, uint64_t* value_result, uint64_t idx, register_uint64_t* reg, SHORT_STDPARAMS);

void extern_register_write__i8(int8_t x, int idx, int8_t value, register_int8_t* reg, SHORT_STDPARAMS);
void extern_register_write__i16(int16_t x, int idx, int16_t value, register_int16_t* reg, SHORT_STDPARAMS);
void extern_register_write__i32(int32_t x, int idx, int32_t value, register_int32_t* reg, SHORT_STDPARAMS);
void extern_register_write__i64(int64_t x, int idx, int64_t value, register_int64_t* reg, SHORT_STDPARAMS);
void extern_register_write__u8(uint8_t x, int idx, uint8_t value, register_uint8_t* reg, SHORT_STDPARAMS);
void extern_register_write__u16(uint16_t x, int idx, uint16_t value, register_uint16_t* reg, SHORT_STDPARAMS);
void extern_register_write__u32(uint32_t x, int idx, uint32_t value, register_uint32_t* reg, SHORT_STDPARAMS);
void extern_register_write__u64(uint64_t x, int idx, uint64_t value, register_uint64_t* reg, SHORT_STDPARAMS);

void init_register__i8(register_int8_t* reg, int8_t size, SHORT_STDPARAMS);
void init_register__i16(register_int16_t* reg, int16_t size, SHORT_STDPARAMS);
void init_register__i32(register_int32_t* reg, int32_t size, SHORT_STDPARAMS);
void init_register__i64(register_int64_t* reg, int64_t size, SHORT_STDPARAMS);
void init_register__u8(register_uint8_t* reg, uint8_t size, SHORT_STDPARAMS);
void init_register__u16(register_uint16_t* reg, uint16_t size, SHORT_STDPARAMS);
void init_register__u32(register_uint32_t* reg, uint32_t size, SHORT_STDPARAMS);
void init_register__u64(register_uint64_t* reg, uint64_t size, SHORT_STDPARAMS);

//=============================================================================
// Counters

void apply_direct_counter(direct_counter_t* smem, uint32_t value, char* table_name, char* smem_type_name, char* smem_name);

void apply_direct_meter(direct_meter_t* smem, uint32_t value, char* table_name, char* smem_type_name, char* smem_name);
void apply_direct_smem_rte_atomic32_t(rte_atomic32_t* smem, uint32_t value, char* table_name, char* smem_type_name, char* smem_name);

// void extern_counter_count(uint32_t counter_array_size, T4P4S_COUNTER_e ct, uint32_t index, counter_t* counter, SHORT_STDPARAMS);
// void extern_direct_counter_count(int counter_type, direct_counter_t* smem, SHORT_STDPARAMS);

// void extern_meter_execute_meter__u32(uint32_t index, T4P4S_METER_e b, uint32_t c, uint8_t* d, meter_t* e, SHORT_STDPARAMS);
// void extern_direct_meter_read__u8(T4P4S_METER_e b, uint32_t c, meter_t* e, SHORT_STDPARAMS);

//=============================================================================
// Locking

typedef rte_spinlock_t lock_t;

#define LOCK(lock) rte_spinlock_lock(lock);

#define UNLOCK(lock) rte_spinlock_unlock(lock);
