// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_v1model.h"
#include "util_packet.h"
#include "util_debug.h"

#include "dpdk_lib.h"
#include "stateful_memory.h"
#include "dpdk_model_v1model_funs.h"

#include "gen_model.h"

#include <rte_ip.h>


extern void do_counter_count(SMEMTYPE(counter)* counter, int index, uint32_t value);

void SHORT_EXTERNCALL1(counter,count)(uint32_t counter_array_size, T4P4S_COUNTER_e ct, uint32_t index, SMEMTYPE(counter)* counter, SHORT_STDPARAMS) {
    index = rte_be_to_cpu_32(index);
    if (index < counter_array_size) {
        do_counter_count(counter, index, ct == enum_CounterType_packets ? 1 : packet_size(pd));
    }
}

void SHORT_EXTERNCALL1(direct_counter,count)(int counter_type, SMEMTYPE(direct_counter)* smem, SHORT_STDPARAMS) {
    debug("    : Executing EXTERNCALL(direct_counter,count)\n");
}


void EXTERNCALL1(meter,execute_meter,u32)(uint32_t index, T4P4S_METER_e b, uint32_t c, uint32_t* d, SMEMTYPE(meter)* e, SHORT_STDPARAMS) {
    debug("    : Executing EXTERNCALL1(meter,execute_meter,u32)#" T4LIT(%d) "\n", index);
}

void EXTERNCALL1(direct_meter,read,u8)(T4P4S_METER_e b, uint8_t* result, SMEMTYPE(direct_meter)* e, SHORT_STDPARAMS) {
    debug("    : Executing EXTERNCALL1(direct_meter,read,u8)\n");
}

void EXTERNCALL1(direct_meter,read,u16)(T4P4S_METER_e b, uint16_t* result, SMEMTYPE(direct_meter)* e, SHORT_STDPARAMS) {
    debug("    : Executing EXTERNCALL1(direct_meter,read,u16)\n");
}

void EXTERNCALL1(direct_meter,read,u32)(T4P4S_METER_e b, uint32_t* result, SMEMTYPE(direct_meter)* e, SHORT_STDPARAMS) {
    debug("    : Executing EXTERNCALL1(direct_meter,read,u32)\n");
}


// Register: init

void SHORT_EXTERNCALL1(init_registe,_i8)(register_int8_t* reg, int8_t size, SHORT_STDPARAMS)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}

void SHORT_EXTERNCALL1(init_register,i16)(register_int16_t* reg, int16_t size, SHORT_STDPARAMS)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}

void SHORT_EXTERNCALL1(init_register,i32)(register_int32_t* reg, int32_t size, SHORT_STDPARAMS)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}

void SHORT_EXTERNCALL1(init_register,i64)(register_int64_t* reg, int64_t size, SHORT_STDPARAMS)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}

void SHORT_EXTERNCALL1(init_register,u8)(register_uint8_t* reg, uint8_t size, SHORT_STDPARAMS)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}

void SHORT_EXTERNCALL1(init_register,u16)(register_uint16_t* reg, uint16_t size, SHORT_STDPARAMS)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}

void SHORT_EXTERNCALL1(init_register,u32)(register_uint32_t* reg, uint32_t size, SHORT_STDPARAMS)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}

void SHORT_EXTERNCALL1(init_register,u64)(register_uint64_t* reg, uint64_t size, SHORT_STDPARAMS)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}


// extern void extern_register_read_u8(register_uint8_t* reg, uint8_t* value_result, uint8_t idx);
// extern void extern_register_write_u8(register_uint8_t* reg, int idx, uint8_t value);

// extern void extern_register_read_u16(register_uint16_t* reg, uint16_t* value_result, uint16_t idx);
// extern void extern_register_write_u16(register_uint16_t* reg, int idx, uint16_t value);

// extern void extern_register_read_u32(register_uint32_t* reg, uint32_t* value_result, uint32_t idx);
// extern void extern_register_write_u32(register_uint32_t* reg, int idx, uint32_t value);

// Register: read

void EXTERNCALL1(register,read,i8)(int8_t x, int8_t* value_result, int8_t idx, register_int8_t* reg, SHORT_STDPARAMS) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(i8) " = " T4LIT(0x%02x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

void EXTERNCALL1(register,read,i16)(int16_t x, int16_t* value_result, int16_t idx, register_int16_t* reg, SHORT_STDPARAMS) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(i16) " = " T4LIT(0x%04x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

void EXTERNCALL1(register,read,i32)(int32_t x, int32_t* value_result, int32_t idx, register_int32_t* reg, SHORT_STDPARAMS) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(i32) " = " T4LIT(0x%08x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

void EXTERNCALL1(register,read,i64)(int64_t x, int64_t* value_result, int64_t idx, register_int64_t* reg, SHORT_STDPARAMS) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%ld) "] = " T4LIT(%ld) "/" T4LIT(i64) " = " T4LIT(0x%016lx,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

void EXTERNCALL1(register,read,u8)(uint8_t x, uint8_t* value_result, uint8_t idx, register_uint8_t* reg, SHORT_STDPARAMS) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%u) "/" T4LIT(u8) " = " T4LIT(0x%02x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

void EXTERNCALL1(register,read,u16)(uint16_t x, uint16_t* value_result, uint16_t idx, register_uint16_t* reg, SHORT_STDPARAMS) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%u) "/" T4LIT(u16) " = " T4LIT(0x%04x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

void EXTERNCALL1(register,read,u32)(uint32_t x, uint32_t* value_result, uint32_t idx, register_uint32_t* reg, SHORT_STDPARAMS) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%u) "/" T4LIT(u32) " = " T4LIT(0x%08x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

void EXTERNCALL1(register,read,u64)(uint64_t x, uint64_t* value_result, uint64_t idx, register_uint64_t* reg, SHORT_STDPARAMS) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%ld) "] = " T4LIT(%lu) "/" T4LIT(u64) " = " T4LIT(0x%016lx,bytes) "\n", reg->name, idx, reg->value, reg->value);
}


// Register: write

void EXTERNCALL1(register,write,i8)(int8_t x, int idx, int8_t value, register_int8_t* reg, SHORT_STDPARAMS) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(i8) " = " T4LIT(0x%02x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}
void EXTERNCALL1(register,write,i16)(int16_t x, int idx, int16_t value, register_int16_t* reg, SHORT_STDPARAMS) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(i16) " = " T4LIT(0x%04x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}
void EXTERNCALL1(register,write,i32)(int32_t x, int idx, int32_t value, register_int32_t* reg, SHORT_STDPARAMS) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(i32) " = " T4LIT(0x%08x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}
void EXTERNCALL1(register,write,i64)(int64_t x, int idx, int64_t value, register_int64_t* reg, SHORT_STDPARAMS) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%ld) "/" T4LIT(i64) " = " T4LIT(0x%016lx,bytes) "\n", reg->name, idx, reg->value, reg->value);
}
void EXTERNCALL1(register,write,u8)(uint8_t x, int idx, uint8_t value, register_uint8_t* reg, SHORT_STDPARAMS) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(u8) " = " T4LIT(0x%02x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}
void EXTERNCALL1(register,write,u16)(uint16_t x, int idx, uint16_t value, register_uint16_t* reg, SHORT_STDPARAMS) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(u16) " = " T4LIT(0x%04x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}
void EXTERNCALL1(register,write,u32)(uint32_t x, int idx, uint32_t value, register_uint32_t* reg, SHORT_STDPARAMS) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(u32) " = " T4LIT(0x%08x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}
void EXTERNCALL1(register,write,u64)(uint64_t x, int idx, uint64_t value, register_uint64_t* reg, SHORT_STDPARAMS) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%lu) "/" T4LIT(u64) " = " T4LIT(0x%016lx,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

