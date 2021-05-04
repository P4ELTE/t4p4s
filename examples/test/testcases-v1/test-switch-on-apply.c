// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        {FAKE_PKT, 0, 1, FDATA("00"), NO_CTL_REPLY, 0, FDATA("00")},
        {FAKE_PKT, 0, 1, FDATA("12"), NO_CTL_REPLY, 0, FDATA("12")},
        {FAKE_PKT, 0, 1, FDATA("34"), NO_CTL_REPLY, 0, FDATA("34")},
        {FAKE_PKT, 0, 1, FDATA("AA"), NO_CTL_REPLY, 0, FDATA("34")},
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
