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
#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__ 1

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

typedef void* controller;
typedef void (*digest_handler)(void*); /* callback for handling a digest received from the switch*/
typedef void (*initialize)(); /* Init script that delegates rules when a switch connects to the controller */

controller create_controller(uint16_t port, int number_of_threads, digest_handler dh);
controller create_controller_with_init(uint16_t port, int number_of_threads, digest_handler dh, initialize init);

void destroy_controller(controller c);

void execute_controller(controller c);
int send_p4_msg(controller c, char* buffer, int length);

#endif
