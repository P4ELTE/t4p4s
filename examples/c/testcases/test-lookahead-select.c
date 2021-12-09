// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = SINGLE_LCORE(
    SLOW(0, 0, OUT("00"), OUT("00"), OUT("00"), OUT("00"), OUT("0000"), OUT("0000"), OUT("00000000"), "00000000"),
    SLOW(0, 0, OUT("01"), OUT("04"), OUT("40"), OUT("80"), OUT("0100"), OUT("8000"), OUT("00010000"), "80000000")
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test", &t4p4s_testcase_test, "v1model" },
    { "psa",  &t4p4s_testcase_test, "psa" },
    TEST_SUITE_END,
};
