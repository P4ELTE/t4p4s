// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_lib.h"
#include "actions.h"
#include "test.h"

#undef PAYLOAD01
#undef PAYLOAD02
#undef PAYLOAD03
#undef PAYLOAD04

#define IP4_01      "4600006a777540003106aa1dc284a263c0a8016b"
#define IP4_01_OPTS "00000000"
#define TCP01       "01bbcf0ea07c2320528c2d6b8018002eaf0d0000"
#define TCP01_OPTS  "0101080a989199cb00280afd"
#define PAYLOAD01   "1703030031ec1d1a9a53b460e99ca9f397165069c993db9c7ac6961a7d821c0c2486c13cd501afde1beda6fc4969dacadd1e318e6b8b"

#define IP4_02      "46c000200000400001024109c0a8016be00000fb"
#define IP4_02_OPTS "94040000"
#define PAYLOAD02   "16000904e00000fb"

#define IP4_03      "45000028e90b40004006f90cc0a8016bc629d07a"
#define IP4_03_OPTS "ec6401bb"
#define PAYLOAD03   "58526ec04df6142d501000e53ee20000"

#define IP4_04      "45000053617240003906852e9765018cc0a8016b"
#define IP4_04_OPTS "01bb9140"
#define TCP04       "c7429b41eca070fd8018003d1e1e00000101080aa"
#define TCP04_OPTS  "33aebd30046882515030300"
#define PAYLOAD04   "1a9eeb2d5965840121f6654196bed15b97011d374df64b5d6689f4"


extern void setdefault_ipv4_fib_lpm_0(ipv4_fib_lpm_0_action_t action);

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        {FAKE_SETDEF, setdefault_ipv4_fib_lpm_0, 0, {"on_miss"}, 0, 0, FDATA("") },

        FAST(0, 0, hETH4(ETH01, ETH02), IP4_01, IP4_01_OPTS, TCP01, TCP01_OPTS, PAYLOAD01),
        // {FAKE_PKT, 0,   0, ETH(ETH01, ETH02, IP4_01, IP4_01_OPTS, TCP01, TCP01_OPTS, PAYLOAD01),
        //  NO_CTL_REPLY,  0, ETH(ETH01, ETH02, IP4_01, IP4_01_OPTS, TCP01, TCP01_OPTS, PAYLOAD01)},
        // {FAKE_PKT, 0, 0, ETH(ETH01, ETH02, IP4_01 IP4_01_OPTS TCP01 TCP01_OPTS PAYLOAD01), 200,  0, ETH(ETH01, ETH02, IP4_01 IP4_01_OPTS TCP01 TCP01_OPTS PAYLOAD01)},
        // {FAKE_PKT, 0, 0, ETH(ETH01, ETH02, IP4_02 IP4_02_OPTS                 PAYLOAD02), 200, 18, ETH(ETH01, ETH02, IP4_02 IP4_02_OPTS                 PAYLOAD02)},
        // {FAKE_PKT, 0, 0, ETH(ETH01, ETH02, IP4_03 IP4_03_OPTS                 PAYLOAD03), 200, 18, ETH(ETH01, ETH02, IP4_03 IP4_03_OPTS                 PAYLOAD03)},
        // {FAKE_PKT, 0, 0, ETH(ETH01, ETH02, IP4_04 IP4_04_OPTS TCP04 TCP04_OPTS PAYLOAD04), 200, 18, ETH(ETH01, ETH02, IP4_04 IP4_04_OPTS TCP04 TCP04_OPTS PAYLOAD04)},

        FEND,
    },

    {
        FEND,
    },
};


testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test },
    TEST_SUITE_END,
};