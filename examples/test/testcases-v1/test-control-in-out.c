// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        // table lookup hits
        {FAKE_PKT, 0, 1, FDATA("AA00"), 200, 123, FDATA("AA22")},
        // table lookup misses
        {FAKE_PKT, 0, 1, FDATA("BB00"), 200, 123, FDATA("BB44")},

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
