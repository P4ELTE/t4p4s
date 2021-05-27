// Copyright 2019 Eotvos Lorand University, Budapest, Hungary
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

#pragma once

#include "dpdk_lib.h"

void dummy_crypto(SHORT_STDPARAMS);
void dummy_crypto__u8s(uint8_buffer_t buf, SHORT_STDPARAMS);
void dummy_crypto__u8s__u8s(uint8_buffer_t buf1, uint8_buffer_t buf2, SHORT_STDPARAMS);
void dummy_crypto__u16s__u32s(uint8_buffer_t buf1, uint8_buffer_t buf2, SHORT_STDPARAMS);

void dummy_crypto__u8(uint8_t u8, SHORT_STDPARAMS);
void dummy_crypto__u16(uint16_t u16, SHORT_STDPARAMS);
void dummy_crypto__u32(uint32_t u32, SHORT_STDPARAMS);
void dummy_crypto__buf(uint8_t* u8s, SHORT_STDPARAMS);
