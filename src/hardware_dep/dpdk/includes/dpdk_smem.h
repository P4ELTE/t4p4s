// Copyright 2016 Eotvos Lorand University, Budapest, Hungary
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
#ifndef DPDK_REG_H
#define DPDK_REG_H

//=============================================================================
// Registers

typedef struct register_int8_s {
    volatile int8_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} register_int8_t;

typedef struct register_int16_s {
    volatile int16_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} register_int16_t;

typedef struct register_int32_s {
    volatile int32_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} register_int32_t;

typedef struct register_int64_s {
    volatile int64_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} register_int64_t;

typedef struct register_uint8_s {
    volatile uint8_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} register_uint8_t;

typedef struct register_uint16_s {
    volatile uint16_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} register_uint16_t;

typedef struct register_uint32_s {
    volatile uint32_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} register_uint32_t;

typedef struct register_uint64_s {
    volatile uint64_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} register_uint64_t;

void extern_register_write_int8_t(register_int8_t* reg, int idx, int8_t value);
void extern_register_read_int8_t(register_int8_t* reg, int8_t* value, uint32_t idx);
void init_register_int8_t(register_int8_t* reg, uint32_t size);

void extern_register_write_int16_t(register_int16_t* reg, int idx, int16_t value);
void extern_register_read_int16_t(register_int16_t* reg, int16_t* value, uint32_t idx);
void init_register_int16_t(register_int16_t* reg, uint32_t size);

void extern_register_write_int32_t(register_int32_t* reg, int idx, int32_t value);
void extern_register_read_int32_t(register_int32_t* reg, int32_t* value, uint32_t idx);
void init_register_int32_t(register_int32_t* reg, uint32_t size);

void extern_register_write_int64_t(register_int64_t* reg, int idx, int64_t value);
void extern_register_read_int64_t(register_int64_t* reg, int64_t* value, uint32_t idx);
void init_register_int64_t(register_int64_t* reg, uint32_t size);

void extern_register_write_uint8_t(register_uint8_t* reg, int idx, uint8_t value);
void extern_register_read_uint8_t(register_uint8_t* reg, uint8_t* value, uint32_t idx);
void init_register_uint8_t(register_uint8_t* reg, uint32_t size);

void extern_register_write_uint16_t(register_uint16_t* reg, int idx, uint16_t value);
void extern_register_read_uint16_t(register_uint16_t* reg, uint16_t* value, uint32_t idx);
void init_register_uint16_t(register_uint16_t* reg, uint32_t size);

void extern_register_write_uint32_t(register_uint32_t* reg, int idx, uint32_t value);
void extern_register_read_uint32_t(register_uint32_t* reg, uint32_t* value, uint32_t idx);
void init_register_uint32_t(register_uint32_t* reg, uint32_t size);

void extern_register_write_uint64_t(register_uint64_t* reg, int idx, uint64_t value);
void extern_register_read_uint64_t(register_uint64_t* reg, uint64_t* value, uint32_t idx);
void init_register_uint64_t(register_uint64_t* reg, uint32_t size);

#define register_read_PARAM1(par) &par
#define register_read_PARAM2(par) par
#define register_write_PARAM1(par) par
#define register_write_PARAM2(par) par

//=============================================================================
// Counters

typedef struct counter_s {
    rte_atomic32_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} counter_t;

typedef struct direct_counter_s {
    rte_atomic32_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} direct_counter_t;

typedef struct direct_meter_uint32_s {
    rte_atomic32_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} direct_meter_uint32_t;

typedef struct direct_meter_uint8_s {
    rte_atomic32_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} direct_meter_uint8_t;

typedef struct direct_meter_s {
    rte_atomic32_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
    // TODO
} direct_meter_t;

typedef struct meter_s {
    rte_atomic32_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} meter_t;

//=============================================================================
// Locking

typedef rte_spinlock_t lock_t;

#define LOCK(lock) rte_spinlock_lock(lock);

#define UNLOCK(lock) rte_spinlock_unlock(lock);


#endif // DPDK_REG_H
