// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        FAST(0, 0, INOUT(hETH4(ETH1A, "112233445566"), hETH4(ETH1A, "111111111111")), "00"),
        FAST(0, 0, INOUT(hETH4(ETH1A, "312233445566"), hETH4(ETH1A, "111111111111")), "00"),
        FAST(0, 0, INOUT(hETH4(ETH1A, "155555555556"), hETH4(ETH1A, "111111111111")), "00"),
        FAST(0, 0, INOUT(hETH4(ETH1A, "220000000006"), hETH4(ETH1A, "220000000006")), "00"),
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
