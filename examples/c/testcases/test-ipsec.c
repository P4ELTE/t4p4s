// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = SINGLE_LCORE(
    #define cHMAC12B "00000000800800df67020100"
    FAST(0, 0, IN(hETH4("DDDDDDDD0000", ETH01)), IN(hMISC(IPv4, "0000000000000000000029f0" "1234" "5678" "0a006363")), IN(PAYLOAD("abcd")),
               OUT(hMISC(IPsec, "003ba40201000000003b646b00000000800001000100ffff0000000000000000000000004200000074000000000000000000" cHMAC12B)))
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test", &t4p4s_testcase_test, "v1model" },
    TEST_SUITE_END,
};
