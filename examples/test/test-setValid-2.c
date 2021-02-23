// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        {FAKE_PKT, 0, 0, ETH(ETH1A, ETH01),   0, 0, FDATA(ETH1A ETH01 ETH01 "0800")},
//        {FAKE_PKT, 0, 0, ETH(ETH1A, ETH02),   0, 0, FDATA(ETH1A ETH02 ETH02 "0800")},
//        {FAKE_PKT, 0, 0, ETH(ETH01, ETH1A),   0, 0, FDATA(ETH01 ETH1A ETH1A "0800")},
//        {FAKE_PKT, 0, 0, ETH(ETH02, ETH1A),   0, 0, FDATA(ETH02 ETH1A ETH1A "0800")},

        FEND,
    },

    {
        // TODO there seems to be a problem with packets from the second lcore

        // {FAKE_PKT, 0, 0, ETH(ETH1A, ETH03),   0, 0, FDATA(ETH1A ETH03 ETH03 "0800")},
//        {FAKE_PKT, 0, 0, ETH(ETH1A, ETH04),   0, 0, FDATA(ETH1A ETH04 ETH04 "0800")},
//        {FAKE_PKT, 0, 0, ETH(ETH03, ETH1A),   0, 0, FDATA(ETH03 ETH1A ETH1A "0800")},
//        {FAKE_PKT, 0, 0, ETH(ETH04, ETH1A),   0, 0, FDATA(ETH04 ETH1A ETH1A "0800")},

        FEND,
    },
};


testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test },
    TEST_SUITE_END,
};
