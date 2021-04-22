// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        {FAKE_PKT, 0, 1, {"00000000", ""}, 200, 12345, FDATA("00", "00", "00", "00", "0000", "0000", "00000000", "00000000")},
        {FAKE_PKT, 0, 1, {"80000000", ""}, 200, 12345, FDATA("01", "04", "40", "80", "0100", "8000", "00010000", "80000000")},
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
