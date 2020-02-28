// Copyright 2018 Eotvos Lorand University, Budapest, Hungary
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "dpdk_lib.h"
#include "stateful_memory.h"
#include <rte_spinlock.h>

#define LOCKED(lock, code) \
    rte_spinlock_lock(lock); \
    code \
    rte_spinlock_unlock(lock);


void apply_direct_smem_rte_atomic32_t(rte_atomic32_t* smem, uint32_t value, char* table_name, char* smem_type_name, char* smem_name) {
    rte_atomic32_add(smem, value);

    debug("    :: Applying " T4LIT(%s) " " T4LIT(%s,smem) "(" T4LIT(%d) ") on table " T4LIT(%s,table) ": new value is " T4LIT(%d) "\n",
          smem_type_name, smem_name, value, table_name, rte_atomic32_read(smem));
}


void extern_counter_count(uint32_t index) {
    debug("    :: Executing extern_counter_count for " T4LIT(counter#%d,smem) "\n", index);
}


void extern_meter_execute_meter_uint32_t(uint32_t index, uint32_t* result) {
    debug("    :: Executing extern_meter_execute_meter_uint32_t#" T4LIT(%d) "\n", index);
}

void extern_meter_execute_meter_uint16_t(uint32_t index, uint16_t* result) {
    debug("    :: Executing extern_meter_execute_meter_uint16_t#" T4LIT(%d) "\n", index);
}

void extern_meter_execute_meter_uint8_t(uint32_t index, uint8_t* result) {
    debug("    :: Executing extern_meter_execute_meter_uint8_t#" T4LIT(%d) "\n", index);
}


void extern_direct_meter_read_uint8_t(uint8_t* result, uint32_t index) {
    debug("    :: Executing extern_direct_meter_read_uint8_t#" T4LIT(%d) "\n", index);
}

void extern_direct_meter_read_uint32_t(uint32_t* result, uint32_t index) {
    debug("    :: Executing extern_direct_meter_read_uint32_t#" T4LIT(%d) "\n", index);
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
}

void extern_register_read_int16_t(register_int16_t* reg, int16_t* value_result, uint32_t idx) { 
    *value_result =  reg[idx].value; 
}

void extern_register_read_int32_t(register_int32_t* reg, int32_t* value_result, uint32_t idx) { 
    *value_result =  reg[idx].value; 
}

void extern_register_read_int64_t(register_int64_t* reg, int64_t* value_result, uint32_t idx) { 
    *value_result =  reg[idx].value; 
}

void extern_register_read_uint8_t(register_uint8_t* reg, uint8_t* value_result, uint32_t idx) { 
    *value_result =  reg[idx].value; 
}

void extern_register_read_uint16_t(register_uint16_t* reg, uint16_t* value_result, uint32_t idx) { 
    *value_result =  reg[idx].value;
    debug("    :: Executing extern_register_read_uint16_t#" T4LIT(%d) " value:" T4LIT(%d) "\n", idx, reg[idx].value);
}

void extern_register_read_uint32_t(register_uint32_t* reg, uint32_t* value_result, uint32_t idx) { 
    *value_result =  reg[idx].value; 
}

void extern_register_read_uint64_t(register_uint64_t* reg, uint64_t* value_result, uint32_t idx) { 
    *value_result =  reg[idx].value; 
}


// Register: write

void extern_register_write_int8_t(register_int8_t* reg, uint32_t idx, int8_t value) { 
    reg[idx].value = value; 
}
void extern_register_write_int16_t(register_int16_t* reg, uint32_t idx, int16_t value) { 
    reg[idx].value = value; 
}
void extern_register_write_int32_t(register_int32_t* reg, uint32_t idx, int32_t value) { 
    reg[idx].value = value; 
}
void extern_register_write_int64_t(register_int64_t* reg, uint32_t idx, int64_t value) { 
    reg[idx].value = value; 
}
void extern_register_write_uint8_t(register_uint8_t* reg, uint32_t idx, uint8_t value) { 
    reg[idx].value = value; 
}
void extern_register_write_uint16_t(register_uint16_t* reg, uint32_t idx, uint16_t value) { 
    reg[idx].value = value;
    debug("    :: Executing extern_register_write_uint16_t#" T4LIT(%d) " value:" T4LIT(%d) "\n", idx, reg[idx].value);
}
void extern_register_write_uint32_t(register_uint32_t* reg, uint32_t idx, uint32_t value) { 
    reg[idx].value = value; 
}
void extern_register_write_uint64_t(register_uint64_t* reg, uint32_t idx, uint64_t value) { 
    reg[idx].value = value; 
}




void init_memories() {
    debug(" :::: Initializing stateful memories\n");
}
