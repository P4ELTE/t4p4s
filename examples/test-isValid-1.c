// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        {FAKE_PKT, 0, 0, ETH(ETH1A, ETH01, ETH04), 200, 0, ETH(ETH1A, ETH01)},
        {FAKE_PKT, 0, 0, ETH(ETH1A, ETH02, ETH04), 200, 0, ETH(ETH1A, ETH02)},
        {FAKE_PKT, 0, 0, ETH(ETH01, ETH1A, ETH04),   0, 0, ETH(ETH01, ETH1A)},
        {FAKE_PKT, 0, 0, ETH(ETH02, ETH1A, ETH04),   0, 0, ETH(ETH02, ETH1A)},

        FEND,
    },

    {
        {FAKE_PKT, 0, 0, ETH(ETH1A, ETH03, ETH01), 200, 0, ETH(ETH1A, ETH03)},
        {FAKE_PKT, 0, 0, ETH(ETH1A, ETH04, ETH01), 200, 0, ETH(ETH1A, ETH04)},
        {FAKE_PKT, 0, 0, ETH(ETH03, ETH1A, ETH01),   0, 0, ETH(ETH03, ETH1A)},
        {FAKE_PKT, 0, 0, ETH(ETH04, ETH1A, ETH01),   0, 0, ETH(ETH04, ETH1A)},

        FEND,
    },
};


testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test  },
    TEST_SUITE_END,
};
