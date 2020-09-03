// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        FSLEEP(200),
        {FAKE_PKT, 0, 0, ETH(ETH01, ETH1A), 200, 0, ETH(ETH01, "acdcaabbbbaa")},
        {FAKE_PKT, 0, 123, ETH(ETH02, ETH1A), 200, 0, ETH(ETH02, ETH1A)},
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
