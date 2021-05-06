// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include <rte_atomic.h>

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

//=============================================================================
// Counters

typedef struct {
    rte_atomic32_t value;
    #ifdef T4P4S_DEBUG
        char name[256];
    #endif
} counter_t;


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

//=============================================================================
// Locking

typedef rte_spinlock_t lock_t;

#define LOCK(lock) rte_spinlock_lock(lock);

#define UNLOCK(lock) rte_spinlock_unlock(lock);
