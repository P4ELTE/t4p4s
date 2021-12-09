// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        WAIT_FOR_CTL,
        {FAKE_PKT, 0, 0, ETH("DDDDDDDD0000", ETH01, "000000000000000000000150", "12345678", "96000102"), CTL_REPLIES, 0, ETH("AAAAABBA0001", "EEEEEDDA0001", "0000000000000000FF000150", "12345678", "96000102")},
        {FAKE_PKT, 0, 0, ETH(ETH1A, ETH04, IPV4_FFFF), CTL_REPLIES, 18, ETH(ETH01, ETH1A, IPV4_FFFF)},
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
