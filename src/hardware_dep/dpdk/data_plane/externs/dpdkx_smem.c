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





extern void gen_init_smems();

void init_memories() {
    debug(" :::: Init stateful memories\n");
    gen_init_smems();
}
