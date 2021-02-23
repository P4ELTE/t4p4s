// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        {FAKE_PKT, 0, 0, ETH(ETH1A, ETH01), NO_CTL_REPLY, 0, FDATA(ETH1A, "0800")},
        {FAKE_PKT, 0, 0, ETH(ETH1A, ETH02), NO_CTL_REPLY, 0, FDATA(ETH1A, "0800")},
        {FAKE_PKT, 0, 0, ETH(ETH01, ETH1A), NO_CTL_REPLY, 0, FDATA(ETH01, "0800")},
        {FAKE_PKT, 0, 0, ETH(ETH02, ETH1A), NO_CTL_REPLY, 0, FDATA(ETH02, "0800")},

        FEND,
    },

    {
        {FAKE_PKT, 0, 0, ETH(ETH1A, ETH03), NO_CTL_REPLY, 0, FDATA(ETH1A, "0800")},
        {FAKE_PKT, 0, 0, ETH(ETH1A, ETH04), NO_CTL_REPLY, 0, FDATA(ETH1A, "0800")},
        {FAKE_PKT, 0, 0, ETH(ETH03, ETH1A), NO_CTL_REPLY, 0, FDATA(ETH03, "0800")},
        {FAKE_PKT, 0, 0, ETH(ETH04, ETH1A), NO_CTL_REPLY, 0, FDATA(ETH04, "0800")},

        FEND,
    },
};


testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test },
    TEST_SUITE_END,
};
