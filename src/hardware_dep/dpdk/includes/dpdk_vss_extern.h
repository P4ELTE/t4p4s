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

#ifndef __DPDK_VSS_EXTERN_H_
#define __DPDK_VSS_EXTERN_H_

/*******************************************************************************
extern Checksum16 {
    Checksum16();
    bit<16> get<D>(in D data);
}
*******************************************************************************/

struct Checksum16 {
    uint16_t (*get) (uint8_t* data, int data_size, packet_descriptor_t* pd, lookup_table_t** tables);
};

void Checksum16_init(struct Checksum16* x);
uint16_t Checksum16_get(uint8_t* data, int size, packet_descriptor_t* pd, lookup_table_t** tables);

#endif // __DPDK_VSS_EXTERN_H_
