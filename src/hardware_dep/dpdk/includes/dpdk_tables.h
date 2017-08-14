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
#ifndef DPDK_TABLES_H
#define DPDK_TABLES_H

#include <rte_version.h>    // for conditional compilation

#if RTE_VERSION >= RTE_VERSION_NUM(17,05,0,0)
typedef uint32_t table_index_t;
#else
typedef uint8_t table_index_t;
#endif

typedef struct extended_table_s {
    void*          rte_table;
    table_index_t  size;
    uint8_t**      content;
} extended_table_t;

//=============================================================================
// Table size limits

#ifdef RTE_ARCH_X86_64
#define HASH_ENTRIES		1024
#else
#define HASH_ENTRIES		1024
#endif
#define LPM_MAX_RULES         1024
#define LPM6_NUMBER_TBL8S (1 << 16)

#endif
