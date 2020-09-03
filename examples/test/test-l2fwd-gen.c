// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        FSLEEP(200),
        {FAKE_PKT, 0, 1, {ETH01, ETH1A, "0800", ETH01, ETH1A, "0800", ETH01, ETH1A, "0800", ""}, 200, 11, {ETH01, ETH1A, "0800", ETH01, ETH1A, "0800", ETH01, ETH1A, "0800", ""}},
        {FAKE_PKT, 0, 1, {ETH02, ETH1A, "0800", ETH01, ETH1A, "0800", ETH01, ETH1A, "0800", ""}, 200, 22, {ETH02, ETH1A, "0800", ETH01, ETH1A, "0800", ""}},
        {FAKE_PKT, 0, 1, {ETH03, ETH1A, "0800", ETH01, ETH1A, "0800", ETH01, ETH1A, "0800", ""}, 200, 33, {ETH03, ETH1A, "0800", ETH01, ETH1A, "0800", ""}},
        {FAKE_PKT, 0, 1, {ETH04, ETH1A, "0800", ETH01, ETH1A, "0800", ETH01, ETH1A, "0800", ""}, 200, 44, {ETH04, ETH1A, "0800", ETH01, ETH1A, "0800", ""}},
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
