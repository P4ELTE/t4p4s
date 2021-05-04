// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        {FAKE_PKT, 0, 1, ETH(ETH1A, "112233445566", "00"), 200, 0, ETH(ETH1A, "111111111111", "00")},
        {FAKE_PKT, 0, 1, ETH(ETH1A, "110000000000", "00"), 200, 0, ETH(ETH1A, "555555555555", "00")},
        {FAKE_PKT, 0, 1, ETH(ETH1A, "113300000000", "00"), 200, 0, ETH(ETH1A, "999999999999", "00")},
        {FAKE_PKT, 0, 1, ETH(ETH1A, "220000000000", "00"), 200, 0, ETH(ETH1A, "220000000000", "00")},
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
