// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        // 0b1_10101010_1000_00_01_0000000 = 1 0xAA 8 0 1 <padding>
        {FAKE_PKT, 0, 1, FDATA("D54080"), NO_CTL_REPLY, 12345, FDATA("7FFF80")},
        FEND,
    },
    {
        FEND,
    },
};

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test, "psa" },
    TEST_SUITE_END,
};
