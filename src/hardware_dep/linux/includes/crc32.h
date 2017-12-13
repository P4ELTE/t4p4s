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

#ifndef CRC32_H
#define CRC32_H

#include "compiler.h"

#include <stddef.h>
#include <stdint.h>

#if CRC32_INSTRUCTION_SUPPORTED
#include <nmmintrin.h>
#endif

uint32_t crc32(const void* data, size_t size, uint32_t init);

#if CRC32_INSTRUCTION_SUPPORTED
#define CRC32_SSE_NAME(data_size) crc32_sse_##data_size##_byte
#define DECLARE_CRC32_SSE(data_size) uint32_t CRC32_SSE_NAME(data_size)(const void* data, unused size_t size, uint32_t init)
#define DEFINE_CRC32_SSE(data_size) __attribute__((__optimize__("unroll-loops"))) DECLARE_CRC32_SSE(data_size)\
{\
    const uint8_t* const byte = data;\
    uint32_t crc = init;\
    unsigned int i;\
    \
    for (i = 0; i < (data_size); ++i)\
        crc = _mm_crc32_u8(crc, byte[i]);\
    \
    return crc;\
}

uint32_t crc32_sse(const void* data, size_t size, uint32_t init);
#endif

#endif
