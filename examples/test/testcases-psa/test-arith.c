// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        {FAKE_PKT, 0, 1, FDATA("00000000", "00000000"), NO_CTL_REPLY, 12345, FDATA("00000000")},
        {FAKE_PKT, 0, 1, FDATA("000000ff", "00000001"), NO_CTL_REPLY, 12345, FDATA("00000100")},
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
