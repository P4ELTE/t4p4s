// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] =
    SINGLE_LCORE(
        FAST(0, 11, hETH4(ETH01, ETH1A), hIP4_0)
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test, "v1model" },
    TEST_SUITE_END,
};
