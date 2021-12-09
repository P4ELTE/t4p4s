// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        WAIT_FOR_CTL,
        {FAKE_PKT, 0, 0, ETH("DDDDDDDD0000", ETH01, "000000000000000000000000", "12345678", "96000102"), 200, 15, ETH(L3_MAC1, L3_MAC2, "000000000000000000000000", "12345678", "96000102")},
        FEND,
    },

    {
        FEND,
    },
};

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test, "v1model" },
    TEST_SUITE_END,
};
