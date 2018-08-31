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


#define LOCKED(lock, code) \
    rte_spinlock_lock(lock); \
    code \
    rte_spinlock_unlock(lock);


void apply_direct_smem_rte_atomic32_t(rte_atomic32_t* smem, uint32_t value, char* table_name, char* smem_type_name, char* smem_name) {
    rte_atomic32_add(smem, value);

    debug("Applying %s %s(%d) on table %s: new value is %d\n",
          smem_type_name, smem_name, value, table_name, rte_atomic32_read(smem));
}


void extern_counter_count(uint32_t index) {
    debug("Executing extern_counter_count for counter#%d\n", index);
}


void extern_meter_execute_meter_uint32_t(uint32_t index, uint32_t* result) {
    debug("Executing extern_meter_execute_meter_uint32_t#%d\n", index);
}

void extern_meter_execute_meter_uint16_t(uint32_t index, uint16_t* result) {
    debug("Executing extern_meter_execute_meter_uint16_t#%d\n", index);
}

void extern_meter_execute_meter_uint8_t(uint32_t index, uint8_t* result) {
    debug("Executing extern_meter_execute_meter_uint8_t#%d\n", index);
}


void extern_direct_meter_read_uint8_t(uint8_t* result, uint32_t index) {
    debug("Executing extern_direct_meter_read_uint8_t#%d\n", index);
}

void extern_direct_meter_read_uint32_t(uint32_t* result, uint32_t index) {
    debug("Executing extern_direct_meter_read_uint32_t#%d\n", index);
}



void extern_register_read_uint8_t(uint8_t* result, uint32_t index) {
    debug("Executing extern_register_read_uint8_t#%d\n", index);
}

void extern_register_read_uint16_t(uint16_t* result, uint32_t index) {
    debug("Executing extern_register_read_uint16_t#%d\n", index);
}

void extern_register_read_uint32_t(uint32_t* result, uint32_t index) {
    debug("Executing extern_register_read_uint32_t#%d\n", index);
}



void extern_register_write_uint8_t(uint32_t index, uint8_t data) {
    debug("Executing extern_register_write_uint8_t#%d\n", index);
}

void extern_register_write_uint16_t(uint32_t index, uint16_t data) {
    debug("Executing extern_register_write_uint16_t#%d\n", index);
}

void extern_register_write_uint32_t(uint32_t index, uint32_t data) {
    debug("Executing extern_register_write_uint32_t#%d\n", index);
}


void init_memories() {
}
