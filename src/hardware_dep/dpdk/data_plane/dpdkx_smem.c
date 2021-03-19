// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_lib.h"
#include "stateful_memory.h"


void apply_direct_meter(direct_meter_t* smem, uint32_t value, char* table_name, char* smem_type_name, char* smem_name) {
    rte_atomic32_add(&(smem->value), value);

    debug("    : Applying " T4LIT(%s) " " T4LIT(%s,smem) "(" T4LIT(%d) ") on table " T4LIT(%s,table) ": new value is " T4LIT(%d) "\n",
          smem_type_name, smem_name, value, table_name, rte_atomic32_read(&(smem->value)));
}


void apply_direct_counter(direct_counter_t* smem, uint32_t value, char* table_name, char* smem_type_name, char* smem_name) {
    rte_atomic32_add(&(smem->value), value);

    debug("    : Applying " T4LIT(%s) " " T4LIT(%s,smem) "(" T4LIT(%d) ") on table " T4LIT(%s,table) ": new value is " T4LIT(%d) "\n",
          smem_type_name, smem_name, value, table_name, rte_atomic32_read(&(smem->value)));
}

void apply_direct_smem_rte_atomic32_t(rte_atomic32_t* smem, uint32_t value, char* table_name, char* smem_type_name, char* smem_name) {
    rte_atomic32_add(smem, value);

    debug("    : Applying " T4LIT(%s) " " T4LIT(%s,smem) "(" T4LIT(%d) ") on table " T4LIT(%s,table) ": new value is " T4LIT(%d) "\n",
          smem_type_name, smem_name, value, table_name, rte_atomic32_read(smem));
}


void do_counter_count(counter_t* counter, int index, uint32_t value) {
    rte_atomic32_add(&(counter[index].value), value);
    #ifdef T4P4S_DEBUG
        debug("    : Counter " T4LIT(%s[%d],smem) " += " T4LIT(%d) " = " T4LIT(%d,bytes) "\n", counter->name, index, value, rte_atomic32_read(&(counter[index].value)));
    #endif
}


void extern_Counter_count(counter_t* counter, int index, uint32_t value) {
    do_counter_count(counter, index, value);
}


void extern_meter_execute_meter_uint32_t(uint32_t index, uint32_t* result) {
    debug("    : Executing extern_meter_execute_meter_uint32_t#" T4LIT(%d) "\n", index);
}

void extern_meter_execute_meter_uint16_t(uint32_t index, uint16_t* result) {
    debug("    : Executing extern_meter_execute_meter_uint16_t#" T4LIT(%d) "\n", index);
}

void extern_meter_execute_meter_uint8_t(meter_t* meter, uint32_t index, uint8_t* result) {
    debug("    : Executing extern_meter_execute_meter_uint8_t#" T4LIT(%d) "\n", index);
}


void extern_direct_meter_read_uint8_t(uint8_t* result, uint32_t index) {
    debug("    : Executing extern_direct_meter_read_uint8_t#" T4LIT(%d) "\n", index);
}

void extern_direct_meter_read_uint32_t(uint32_t* result, uint32_t index) {
    debug("    : Executing extern_direct_meter_read_uint32_t#" T4LIT(%d) "\n", index);
}

// Register: init

void init_register_int8_t(register_int8_t* reg, uint32_t size)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}

void init_register_int16_t(register_int16_t* reg, uint32_t size)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}

void init_register_int32_t(register_int32_t* reg, uint32_t size)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}

void init_register_int64_t(register_int64_t* reg, uint32_t size)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}

void init_register_uint8_t(register_uint8_t* reg, uint32_t size)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}

void init_register_uint16_t(register_uint16_t* reg, uint32_t size)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}

void init_register_uint32_t(register_uint32_t* reg, uint32_t size)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}

void init_register_uint64_t(register_uint64_t* reg, uint32_t size)  {
    for (uint32_t i=0;i<size;++i) reg[i].value = 0;
}


// Register: read

void extern_register_read_int8_t(register_int8_t* reg, int8_t* value_result, uint32_t idx) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(i8) " = " T4LIT(0x%02x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

void extern_register_read_int16_t(register_int16_t* reg, int16_t* value_result, uint32_t idx) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(i16) " = " T4LIT(0x%04x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

void extern_register_read_int32_t(register_int32_t* reg, int32_t* value_result, uint32_t idx) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(i32) " = " T4LIT(0x%08x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

void extern_register_read_int64_t(register_int64_t* reg, int64_t* value_result, uint32_t idx) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%ld) "/" T4LIT(i64) " = " T4LIT(0x%016lx,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

void extern_register_read_uint8_t(register_uint8_t* reg, uint8_t* value_result, uint32_t idx) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%u) "/" T4LIT(u8) " = " T4LIT(0x%02x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

void extern_register_read_uint16_t(register_uint16_t* reg, uint16_t* value_result, uint32_t idx) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%u) "/" T4LIT(u16) " = " T4LIT(0x%04x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

void extern_register_read_uint32_t(register_uint32_t* reg, uint32_t* value_result, uint32_t idx) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%u) "/" T4LIT(u32) " = " T4LIT(0x%08x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}

void extern_register_read_uint64_t(register_uint64_t* reg, uint64_t* value_result, uint32_t idx) {
    *value_result =  reg[idx].value;
    debug("    : Read from register " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%lu) "/" T4LIT(u64) " = " T4LIT(0x%016lx,bytes) "\n", reg->name, idx, reg->value, reg->value);
}


// Register: write

void extern_register_write_int8_t(register_int8_t* reg, int idx, int8_t value) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(i8) " = " T4LIT(0x%02x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}
void extern_register_write_int16_t(register_int16_t* reg, int idx, int16_t value) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(i16) " = " T4LIT(0x%04x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}
void extern_register_write_int32_t(register_int32_t* reg, int idx, int32_t value) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(i32) " = " T4LIT(0x%08x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}
void extern_register_write_int64_t(register_int64_t* reg, int idx, int64_t value) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%ld) "/" T4LIT(i64) " = " T4LIT(0x%016lx,bytes) "\n", reg->name, idx, reg->value, reg->value);
}
void extern_register_write_uint8_t(register_uint8_t* reg, int idx, uint8_t value) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(u8) " = " T4LIT(0x%02x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}
void extern_register_write_uint16_t(register_uint16_t* reg, int idx, uint16_t value) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(u16) " = " T4LIT(0x%04x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}
void extern_register_write_uint32_t(register_uint32_t* reg, int idx, uint32_t value) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%d) "/" T4LIT(u32) " = " T4LIT(0x%08x,bytes) "\n", reg->name, idx, reg->value, reg->value);
}
void extern_register_write_uint64_t(register_uint64_t* reg, int idx, uint64_t value) {
    reg[idx].value = value;
    debug("    : Writing register: " T4LIT(%s,smem) "[" T4LIT(%d) "] = " T4LIT(%lu) "/" T4LIT(u64) " = " T4LIT(0x%016lx,bytes) "\n", reg->name, idx, reg->value, reg->value);
}



extern void gen_init_smems();

void init_memories() {
    debug(" :::: Init stateful memories\n");
    gen_init_smems();
}
