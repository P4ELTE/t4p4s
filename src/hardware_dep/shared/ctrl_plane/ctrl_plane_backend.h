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
#ifndef __BACKEND_H__
#define __BACKEND_H__ 1

#include <stdint.h>
#include "handlers.h"

typedef void* backend;
typedef void* digest;


backend create_backend(int num_of_threads, int queue_size, char* controller_name, int controller_port, p4_msg_callback cb);
void destroy_backend(backend bg);
int send_digest(backend bg, digest d, uint32_t receiver_id);
void launch_backend(backend bg);
void stop_backend(backend bg);

digest create_digest(backend bg, char* name);
digest add_digest_field(digest d, void* value, uint32_t length);


#endif
