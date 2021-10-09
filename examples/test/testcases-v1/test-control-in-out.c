// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] =
    SINGLE_LCORE(
        // table lookup hits
        FAST(1, 123, INOUT("AA00", "AA22")),
        WAIT_FOR_CTL,
        WAIT_FOR_CTL,
        WAIT_FOR_CTL,
        // table lookup misses
        FAST(1, 123, INOUT("BB00", "BB11"))
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test },
    TEST_SUITE_END,
};
