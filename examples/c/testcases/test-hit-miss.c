// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = SINGLE_LCORE(
    FAST(0, 0, IN("00"), OUT(hMISC(hit, "01")), OUT(hMISC(miss, "00"))),
    FAST(0, 0, IN("01"), OUT(hMISC(hit, "00")), OUT(hMISC(miss, "01")))
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test", &t4p4s_testcase_test, "v1model" },
    { "psa",  &t4p4s_testcase_test, "psa" },
    TEST_SUITE_END,
};
