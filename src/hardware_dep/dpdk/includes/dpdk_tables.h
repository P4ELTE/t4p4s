// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include <rte_version.h>    // for conditional compilation

#if RTE_VERSION >= RTE_VERSION_NUM(17,05,0,0)
typedef uint32_t table_index_t;
#else
typedef uint8_t table_index_t;
#endif

typedef struct {
    void*          rte_table;
    table_index_t  size;
    uint8_t**      content;
} extended_table_t;

//=============================================================================

void rte_exit_with_errno(const char* table_type, const char* table_name);

//=============================================================================
// Table size limits

#ifdef RTE_ARCH_X86_64
#define HASH_ENTRIES		1024
#else
#define HASH_ENTRIES		1024
#endif
#define LPM_MAX_RULES         1024
#define LPM6_NUMBER_TBL8S (1 << 16)

// #define TABLE_MAX 100000
#define TABLE_MAX 250000
