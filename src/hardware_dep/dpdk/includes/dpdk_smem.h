// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#pragma once

//=============================================================================
// Registers

typedef struct {
    volatile int8_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} register_int8_t;

typedef struct {
    volatile int16_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} register_int16_t;

typedef struct {
    volatile int32_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} register_int32_t;

typedef struct {
    volatile int64_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} register_int64_t;

typedef struct {
    volatile uint8_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} register_uint8_t;

typedef struct {
    volatile uint16_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} register_uint16_t;

typedef struct {
    volatile uint32_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} register_uint32_t;

typedef struct {
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

typedef struct {
    rte_atomic32_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} counter_t;


// TODO improve this
typedef counter_t Counter_uint64_t_uint1_t;
typedef counter_t Counter_t;



typedef struct {
    rte_atomic32_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} direct_counter_t;

typedef struct {
    rte_atomic32_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} direct_meter_uint32_t;

typedef struct {
    rte_atomic32_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} direct_meter_uint8_t;

typedef struct {
    rte_atomic32_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
    // TODO
} direct_meter_t;

typedef struct {
    rte_atomic32_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} meter_t;

void apply_direct_meter(direct_meter_t* smem, uint32_t value, char* table_name, char* smem_type_name, char* smem_name);
void apply_direct_counter(direct_counter_t* smem, uint32_t value, char* table_name, char* smem_type_name, char* smem_name);
void apply_direct_smem_rte_atomic32_t(rte_atomic32_t* smem, uint32_t value, char* table_name, char* smem_type_name, char* smem_name);

//=============================================================================
// Locking

typedef rte_spinlock_t lock_t;

#define LOCK(lock) rte_spinlock_lock(lock);

#define UNLOCK(lock) rte_spinlock_unlock(lock);
