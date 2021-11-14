// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
		FAST(0, 0, IN(hETH4(ETH1A, ETH01)), OUT(ETH1A), OUT("0800")),
        FAST(0, 0, IN(hETH4(ETH1A, ETH02)), OUT(ETH1A), OUT("0800")),
        FAST(0, 0, IN(hETH4(ETH01, ETH1A)), OUT(ETH01), OUT("0800")),
        FAST(0, 0, IN(hETH4(ETH02, ETH1A)), OUT(ETH02), OUT("0800")),
		
        FEND,
    },

    {
		FAST(0, 0, IN(hETH4(ETH1A, ETH03)), OUT(ETH1A), OUT("0800")),
        FAST(0, 0, IN(hETH4(ETH1A, ETH04)), OUT(ETH1A), OUT("0800")),
        FAST(0, 0, IN(hETH4(ETH03, ETH1A)), OUT(ETH03), OUT("0800")),
        FAST(0, 0, IN(hETH4(ETH04, ETH1A)), OUT(ETH04), OUT("0800")),
		
        FEND,
    },
};


testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test, "v1model" },
    TEST_SUITE_END,
};
