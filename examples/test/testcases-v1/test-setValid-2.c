// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
	
	{
		FAST(0, 0, ETH1A, "12345678ABCD", IN(cIPV4), OUT("12345678ABCD"), OUT("0800")),
        FAST(0, 0, ETH1A, ETH01, IN(cIPV4), OUT(ETH01), OUT("0800")),
        FAST(0, 0, ETH1A, ETH02, IN(cIPV4), OUT(ETH02), OUT("0800")),
        FAST(0, 0, ETH01, ETH1A, IN(cIPV4), OUT(ETH1A), OUT("0800")),
        FAST(0, 0, ETH02, ETH1A, IN(cIPV4), OUT(ETH1A), OUT("0800")),

        FEND,
    },

    {
        FAST(0, 0, ETH1A, ETH03, IN(cIPV4), OUT(ETH03), OUT("0800")),
        FAST(0, 0, ETH1A, ETH04, IN(cIPV4), OUT(ETH04), OUT("0800")),
        FAST(0, 0, ETH03, ETH1A, IN(cIPV4), OUT(ETH1A), OUT("0800")),
        FAST(0, 0, ETH04, ETH1A, IN(cIPV4), OUT(ETH1A), OUT("0800")),
	
        FEND,
    },
};


testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test },
    TEST_SUITE_END,
};
