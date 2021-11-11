// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        FAST(0, 0, hETH4(ETH1A, ETH01), OUT("eb")),
        FAST(0, 0, hETH4(ETH1A, ETH02), OUT("eb")),
        FAST(0, 0, hETH4(ETH01, ETH1A), OUT("eb")),
        FAST(0, 0, hETH4(ETH02, ETH1A), OUT("eb")),

        FEND,
    },

    {
        FAST(0, 0, hETH4(ETH1A, ETH03), OUT("eb")),
        FAST(0, 0, hETH4(ETH1A, ETH04), OUT("eb")),
        FAST(0, 0, hETH4(ETH03, ETH1A), OUT("eb")),
        FAST(0, 0, hETH4(ETH04, ETH1A), OUT("eb")),

        FEND,
    },
};


testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test },
    TEST_SUITE_END,
};
