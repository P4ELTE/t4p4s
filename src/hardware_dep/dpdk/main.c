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
#include "dpdk_lib.h"

extern uint8_t initialize(int argc, char **argv);
extern void init_control_plane();
extern int launch_dpdk();

int main(int argc, char** argv)
{
    initialize(argc, argv);
    init_control_plane();
    int retval = launch_dpdk();
    printf("Exiting program.\n");
    return retval;
}
