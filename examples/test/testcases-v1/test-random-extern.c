// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        {FAKE_PKT, 0, 1, FDATA("08"), NO_CTL_REPLY, 123, FDATA("2a")},
        {FAKE_PKT, 0, 1, FDATA("10"), NO_CTL_REPLY, 123, FDATA("3412")},
        {FAKE_PKT, 0, 1, FDATA("20"), NO_CTL_REPLY, 123, FDATA("78563412")},
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
