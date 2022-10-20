// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = SINGLE_LCORE(
    FAST(0, DROP, hETH4(ETH02, ETH1A), hIP4(cIPv4, c0x4B, c0x4B))
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test", &t4p4s_testcase_test, "v1model" },
    // fabric is not designed to be compatible with the psa model
    TEST_SUITE_END,
};
