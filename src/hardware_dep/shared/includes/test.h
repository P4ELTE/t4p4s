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


// ------------------------------------------------------
// Fake packet data

enum fake_cmd_e {
    FAKE_PKT,
    FAKE_END,
};

#define MAX_SECTION_COUNT 128

typedef struct fake_cmd_s {
    enum fake_cmd_e action;
    char*           in[MAX_SECTION_COUNT];

    int             sleep_millis;

    int             out_port;
    char*           out[MAX_SECTION_COUNT];
} fake_cmd_t;

// ------------------------------------------------------
// Fake packet data creation helpers

#define FDATA(...)    { __VA_ARGS__, "" }

#define FSLEEP(time)  {FAKE_PKT, FDATA(""), time, 0, FDATA("")}
#define FEND          {FAKE_END, FDATA(""),    0, 0, FDATA("")}

#define ETH(dst, src, ...) FDATA(dst, src, "0800", ##__VA_ARGS__)

#define ETH01 "000001000000"
#define ETH02 "000002000000"
#define ETH03 "000003000000"
#define ETH04 "000004000000"

#define ETH1A "001234567890"

// random payloads

#define PAYLOAD01 "0123456789abcdef"
#define PAYLOAD02 "089789755756"
#define PAYLOAD03 "048989520487"
#define PAYLOAD04 "ffffffffffffff"

#define PAYLOAD11 "0a0a0a0a0a"
#define PAYLOAD12 "a0a0a0a0a0"
#define PAYLOAD13 "00000000"
#define PAYLOAD14 "f00ff00f"

#endif
