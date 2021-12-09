// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = SINGLE_LCORE(
    WAIT_FOR_CTL,
    SLOW(0, 123, hETH4(ETH01, INOUT(ETH1A, "acdcaabbbbaa"))),
    SLOW(123, 0, hETH4(ETH02, ETH1A))
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test", &t4p4s_testcase_test, "v1model" },
    { "psa",  &t4p4s_testcase_test, "psa" },
    TEST_SUITE_END,
};
