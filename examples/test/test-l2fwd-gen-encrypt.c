// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
        {
                FSLEEP(200),
                {FAKE_PKT, 0, 1, ETH(ETH01, ETH1A), 200, 11, ETH(ETH01, ETH1A)},
                {FAKE_PKT, 0, 1, ETH(ETH02, ETH1A), 200, 22, ETH(ETH02, ETH1A)},
                {FAKE_PKT, 0, 1, ETH(ETH03, ETH1A), 200, 33, ETH(ETH03, ETH1A)},
                {FAKE_PKT, 0, 1, ETH(ETH04, ETH1A), 200, 44, ETH(ETH04, ETH1A)},
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
