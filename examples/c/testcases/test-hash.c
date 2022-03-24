// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] =
    SINGLE_LCORE(
        FAST(0, 0, x6h(INOUT("0000", "1234")), INOUT("0000", "4434"), INOUT("0000", "444C"), INOUT("0000", "de4c"), INOUT("0000", "def0"), INOUT(c0x4B, "00001234"), INOUT(c0x4B, "00123456"), x3h(INOUT(c0x4B, "12345678")))
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test", &t4p4s_testcase_test, "v1model" },
    { "psa",  &t4p4s_testcase_test, "psa" },
    TEST_SUITE_END,
};
