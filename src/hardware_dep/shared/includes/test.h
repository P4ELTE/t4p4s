// Copyright 2018 Eotvos Lorand University, Budapest, Hungary
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

#ifndef __TEST_H_
#define __TEST_H_

#include <stdint.h>

// ------------------------------------------------------
// Fake packet data

enum fake_cmd_e {
    FAKE_PKT,
    // set default action
    FAKE_SETDEF,
    FAKE_END,
};

#define MAX_SECTION_COUNT 128

typedef struct fake_cmd_s {
    enum fake_cmd_e action;
    void*           ptr;
    uint32_t        in_port;
    const char*     in[MAX_SECTION_COUNT];

    int             sleep_millis;

    uint32_t        out_port;
    const char*     out[MAX_SECTION_COUNT];
} fake_cmd_t;

// ------------------------------------------------------
// Test suite

typedef struct testcase_s {
    const char* name;
    fake_cmd_t (*steps)[][RTE_MAX_LCORE];
} testcase_t;

#define TEST_SUITE_END { "", 0 }

#define MAX_TESTCASES 128

#define PAUSE_BETWEEN_TESTCASES_MILLIS 500

// ------------------------------------------------------
// Fake packet data creation helpers

#define INIT_WAIT_CONTROLPLANE_SHORT_MILLIS 500
#define INIT_WAIT_CONTROLPLANE_LONG_MILLIS  2000
#define WAIT_OTHER_CORE_PROCESSES_PACKAGES_MILLIS  500
#define WAIT_CONTROLPLANE_REPLY  200


#define FDATA(...)    { __VA_ARGS__, "" }

#define FSLEEP(time)  {FAKE_PKT, 0, 0, FDATA(""), time, 0, FDATA("")}
#define FEND          {FAKE_END, 0, 0, FDATA(""),    0, 0, FDATA("")}

#define UNKNOWN_PKT(src, dst, ...)        {FAKE_PKT, 0, 0, ETH(src, dst, ##__VA_ARGS__), WAIT_CONTROLPLANE_REPLY,    0, ETH(src, dst, ##__VA_ARGS__)}
#define LEARNED_PKT(port, src, dst, ...)  {FAKE_PKT, 0, 0, ETH(src, dst, ##__VA_ARGS__),                       0, port, ETH(src, dst, ##__VA_ARGS__)}

#define ETH(dst, src, ...) FDATA(dst, src, "0800", ##__VA_ARGS__)
#define IPV4(dsteth, dstip, srceth, srcip, ...) ETH(dsteth, srceth, dstip, srcip, ##__VA_ARGS__)

#define IPV4_0000 "0000000000000000000000000000000000000000"

#define ETH01 "000001000000"
#define ETH02 "000002000000"
#define ETH03 "000003000000"
#define ETH04 "000004000000"

#define ETH1A "001234567890"
#define ETH1B "001234567891"

#define LPM_ETH1 "cccccccc0000"
#define LPM_ETH2 "dddddddd0000"

// random payloads

#define PAYLOAD01 "0123456789abcdef"
#define PAYLOAD02 "089789755756"
#define PAYLOAD03 "048989520487"
#define PAYLOAD04 "ffffffffffffff"

#define PAYLOAD11 "0a0a0a0a0a"
#define PAYLOAD12 "a0a0a0a0a0"
#define PAYLOAD13 "00000000"
#define PAYLOAD14 "f00ff00f"

// LPM prefixes

#define LPM1_TOP16B   "9600"
#define LPM2_TOP16B   "3200"

#define T4P4S_BROADCAST_PORT 100

#endif
