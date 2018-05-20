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

#ifndef __UTIL_H_
#define __UTIL_H_

#include <stdint.h>
#include <stdio.h>
#include <rte_common.h>

void dbg_fprint_bytes(FILE* out_file, void* bytes, int byte_count);


#ifdef P4DPDK_DEBUG
    #define __SHORTFILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

    #define MAX_DBG_BYTE_COUNT 100

    #define dbg_bytes(bytes, byte_count, M, ...)   \
        { \
            if (byte_count > MAX_DBG_BYTE_COUNT) { \
                rte_exit(1, "More than %d bytes are read at %s@%d\n", MAX_DBG_BYTE_COUNT, __SHORTFILENAME__, __LINE__); \
            } else { \
                debug(M, ##__VA_ARGS__); \
                dbg_fprint_bytes(stderr, bytes, byte_count); \
                fprintf(stderr, "\n"); \
            } \
        }
#else
    #define dbg_bytes(bytes, byte_count, M, ...)   
#endif

#endif
