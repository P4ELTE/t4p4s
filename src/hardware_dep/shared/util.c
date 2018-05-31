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

#include "backend.h"
#include "util.h"
#include <pthread.h>

pthread_mutex_t dbg_mutex;

void dbg_fprint_bytes(FILE* out_file, void* bytes, int byte_count) {
#ifdef P4DPDK_DEBUG
    pthread_mutex_lock(&dbg_mutex);

    for (int i = 0; i < byte_count; ++i) {
        fprintf(out_file, i%2 == 0 ? "%02x" : "%02x ", ((uint8_t*)bytes)[i]);
    }

    pthread_mutex_unlock(&dbg_mutex);
#endif
}
