// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include "test_testsuite.h"

#define c0x2B  "0000"
#define c0x3B  "000000"
#define c0x4B  c0x2B c0x2B
#define c0x5B  c0x3B c0x2B
#define c0x6B  c0x4B c0x2B
#define c0x7B  c0x4B c0x3B
#define c0x8B  c0x4B c0x4B
#define c0x16B c0x8B c0x8B
#define c0x20B c0x16B c0x4B

// ------------------------------------------------------
// Constants

// empty constants
#define cETH_0 c0x6B
#define cIP4_0 c0x4B

// empty headers
#define IPV6_0000 c0x16B
#define hIP4_0 hIP4("00", cIP4_0, cIP4_0)
#define hTCP_0 hTCP("0000","0000","00000000","00000000","0","0","00","0000",CHKSM0,"0000")

#define ETH01 "000001000000"
#define ETH02 "000002000000"
#define ETH03 "000003000000"
#define ETH04 "000004000000"

#define ETH1A "001234567890"
#define ETH1B "001234567891"

#define LPM_ETH1 "cccccccc0000"
#define LPM_ETH2 "dddddddd0000"

#define IPV4_FFFF "00000000000000000000" "ffff" cIP4_0 cIP4_0

// other MAC addresses

#define L3_MAC1 "D2690FA8399C"
#define L3_MAC2 "D2690F00009C"

// random payloads

#define PAYLOAD01 "0123456789abcdef"
#define PAYLOAD02 "089789755756"
#define PAYLOAD03 "048989520487"
#define PAYLOAD04 "ffffffffffffff"

#define PAYLOAD11 "0a0a0a0a0a"
#define PAYLOAD12 "a0a0a0a0a0"
#define PAYLOAD13 c0x4B
#define PAYLOAD14 "f00ff00f"

#define c78x20B "7878787878787878787878787878787878787878"

// LPM prefixes

#define LPM1_TOP16B   "9600"
#define LPM2_TOP16B   "3200"
