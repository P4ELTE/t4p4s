// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        FAST(0, 0, INOUT(hETH4(ETH1A, ETH01), hETH("123456789ABC", ETH01, "1234"))),

        FEND,
    },

    {
        FAST(0, 0, INOUT(hETH4(ETH1A, ETH03), hETH("123456789ABC", ETH03, "1234")), ETH01),

        FEND,
    },
};


testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test  },
    TEST_SUITE_END,
};
