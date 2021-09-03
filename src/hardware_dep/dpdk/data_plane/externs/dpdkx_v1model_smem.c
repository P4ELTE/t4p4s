// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_v1model.h"
#include "util_packet.h"
#include "util_debug.h"

#include "dpdk_lib.h"
#include "stateful_memory.h"

#include "gen_model.h"

#include <rte_ip.h>


extern void do_counter_count(counter_t* counter, int index, uint32_t value);

void extern_counter_count(uint32_t counter_array_size, T4P4S_COUNTER_e ct, uint32_t index, counter_t* counter, SHORT_STDPARAMS) {
    index = rte_be_to_cpu_32(index);
    if (index < counter_array_size) {
        do_counter_count(counter, index, ct == enum_CounterType_packets ? 1 : packet_size(pd));
    }
}

void extern_direct_counter_count(int counter_type, direct_counter_t* smem, SHORT_STDPARAMS) {
    debug("    : Executing extern_direct_counter_count\n");
}


void extern_meter_execute_meter__u32(uint32_t index, T4P4S_METER_e b, uint32_t c, uint8_t* d, meter_t* e, SHORT_STDPARAMS) {
    debug("    : Executing extern_meter_execute_meter__u32#" T4LIT(%d) "\n", index);
}

void extern_direct_meter_read__u8(T4P4S_METER_e b, uint32_t c, meter_t* e, SHORT_STDPARAMS) {
    debug("    : Executing extern_direct_meter_read__u8\n");
}

void extern_direct_meter_read__u16(T4P4S_METER_e b, uint32_t c, meter_t* e, SHORT_STDPARAMS) {
    debug("    : Executing extern_direct_meter_read__u16\n");
}

void extern_direct_meter_read__u32(T4P4S_METER_e b, uint32_t c, meter_t* e, SHORT_STDPARAMS) {
    debug("    : Executing extern_direct_meter_read__u32\n");
}


// Register: init

void init_register__i8(register_int8_t* reg, int8_t size, SHORT_STDPARAMS)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}

void init_register__i16(register_int16_t* reg, int16_t size, SHORT_STDPARAMS)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}

void init_register__i32(register_int32_t* reg, int32_t size, SHORT_STDPARAMS)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}

void init_register__i64(register_int64_t* reg, int64_t size, SHORT_STDPARAMS)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}

void init_register__u8(register_uint8_t* reg, uint8_t size, SHORT_STDPARAMS)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}

void init_register__u16(register_uint16_t* reg, uint16_t size, SHORT_STDPARAMS)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}

void init_register__u32(register_uint32_t* reg, uint32_t size, SHORT_STDPARAMS)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}

void init_register__u64(register_uint64_t* reg, uint64_t size, SHORT_STDPARAMS)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}


extern void extern_register_read_uint8_t(register_uint8_t* reg, uint8_t* value_result, uint8_t idx);
extern void extern_register_write_uint8_t(register_uint8_t* reg, int idx, uint8_t value);

extern void extern_register_read_uint16_t(register_uint16_t* reg, uint16_t* value_result, uint16_t idx);
extern void extern_register_write_uint16_t(register_uint16_t* reg, int idx, uint16_t value);

extern void extern_register_read_uint32_t(register_uint32_t* reg, uint32_t* value_result, uint32_t idx);
extern void extern_register_write_uint32_t(register_uint32_t* reg, int idx, uint32_t value);

// Register: read

void extern_register_read__i8(int8_t x, int8_t* value_result, int8_t idx, register_int8_t* reg, SHORT_STDPARAMS) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(i8) " = " T4LIT(0x%02x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

void extern_register_read__i16(int16_t x, int16_t* value_result, int16_t idx, register_int16_t* reg, SHORT_STDPARAMS) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(i16) " = " T4LIT(0x%04x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

void extern_register_read__i32(int32_t x, int32_t* value_result, int32_t idx, register_int32_t* reg, SHORT_STDPARAMS) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(i32) " = " T4LIT(0x%08x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

void extern_register_read__i64(int64_t x, int64_t* value_result, int64_t idx, register_int64_t* reg, SHORT_STDPARAMS) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%ld) "] = " T4LIT(%ld) "/" T4LIT(i64) " = " T4LIT(0x%016lx,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

void extern_register_read__u8(uint8_t x, uint8_t* value_result, uint8_t idx, register_uint8_t* reg, SHORT_STDPARAMS) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%u) "/" T4LIT(u8) " = " T4LIT(0x%02x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

void extern_register_read__u16(uint16_t x, uint16_t* value_result, uint16_t idx, register_uint16_t* reg, SHORT_STDPARAMS) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%u) "/" T4LIT(u16) " = " T4LIT(0x%04x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

void extern_register_read__u32(uint32_t x, uint32_t* value_result, uint32_t idx, register_uint32_t* reg, SHORT_STDPARAMS) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%u) "/" T4LIT(u32) " = " T4LIT(0x%08x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

void extern_register_read__u64(uint64_t x, uint64_t* value_result, uint64_t idx, register_uint64_t* reg, SHORT_STDPARAMS) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%ld) "] = " T4LIT(%lu) "/" T4LIT(u64) " = " T4LIT(0x%016lx,bytes) "\n", reg->name, idx, reg->value, reg->value);
}


// Register: write

void extern_register_write__i8(int8_t x, int idx, int8_t value, register_int8_t* reg, SHORT_STDPARAMS) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(i8) " = " T4LIT(0x%02x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}
void extern_register_write__i16(int16_t x, int idx, int16_t value, register_int16_t* reg, SHORT_STDPARAMS) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(i16) " = " T4LIT(0x%04x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}
void extern_register_write__i32(int32_t x, int idx, int32_t value, register_int32_t* reg, SHORT_STDPARAMS) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(i32) " = " T4LIT(0x%08x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}
void extern_register_write__i64(int64_t x, int idx, int64_t value, register_int64_t* reg, SHORT_STDPARAMS) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%ld) "/" T4LIT(i64) " = " T4LIT(0x%016lx,bytes) "\n", reg->name, idx, reg->value, reg->value);
}
void extern_register_write__u8(uint8_t x, int idx, uint8_t value, register_uint8_t* reg, SHORT_STDPARAMS) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(u8) " = " T4LIT(0x%02x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}
void extern_register_write__u16(uint16_t x, int idx, uint16_t value, register_uint16_t* reg, SHORT_STDPARAMS) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(u16) " = " T4LIT(0x%04x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}
void extern_register_write__u32(uint32_t x, int idx, uint32_t value, register_uint32_t* reg, SHORT_STDPARAMS) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(u32) " = " T4LIT(0x%08x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}
void extern_register_write__u64(uint64_t x, int idx, uint64_t value, register_uint64_t* reg, SHORT_STDPARAMS) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%lu) "/" T4LIT(u64) " = " T4LIT(0x%016lx,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

