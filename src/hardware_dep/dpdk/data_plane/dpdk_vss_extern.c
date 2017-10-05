// Copyright 2017 Eotvos Lorand University, Budapest, Hungary
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

#include "backend.h"
#include "dpdk_vss_extern.h"

// extern Checksum16

void Checksum16_init(struct Checksum16* x) {
    x->get = &Checksum16_get;
}

uint16_t csum16_add(uint16_t num1, uint16_t num2) {
    if(num1 == 0) return num2;
    uint32_t tmp_num = num1 + num2;
    while(tmp_num > 0xffff)
        tmp_num = ((tmp_num & 0xffff0000) >> 16) + (tmp_num & 0xffff);
    return (uint16_t)tmp_num;
}

uint16_t Checksum16_get(struct uint8_buffer_t buf, packet_descriptor_t* pd, lookup_table_t** tables) {
    uint32_t res = 0;
    res = csum16_add(res, calculate_csum16(buf.buffer, buf.buffer_size));
    res = (res == 0xffff) ? res : ((~res) & 0xffff);
    return res & 0xffff; // hex((2 ** 16) - 1)
}

