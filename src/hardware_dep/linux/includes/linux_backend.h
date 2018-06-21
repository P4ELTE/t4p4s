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

#ifndef LINUX_BACKEND_H
#define LINUX_BACKEND_H

#include "backend.h"
#include "compiler.h"
#include "config.h"
#include "ethdev.h"
#include "lib.h"
#include "pktbuf.h"

#include <stdint.h>

extern lookup_table_t* tables[NB_TABLES];
extern counter_t* counters[NB_COUNTERS];
extern p4_register_t* registers[NB_REGISTERS];

extern struct ethdev* ports;
extern uint8_t port_count;

extern int init_control_plane(void);
extern void uninitialize(void);

#endif
