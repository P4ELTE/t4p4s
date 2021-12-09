// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = SINGLE_LCORE(
    FAST(0, 0, hETH4(ETH1A, "912233445569"), "00"),
    FAST(0, 0, hETH4(ETH1A, "220000000006"), "00"),

    FAST(0, 0, hETH4(ETH1A, INOUT("112233445566", "111111111111")), "00"),
    FAST(0, 0, hETH4(ETH1A, INOUT("155555555556", "111111111111")), "00")
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test", &t4p4s_testcase_test, "v1model" },
    { "psa",  &t4p4s_testcase_test, "psa" },
    TEST_SUITE_END,
};
