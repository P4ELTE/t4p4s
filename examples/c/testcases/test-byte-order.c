// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = SINGLE_LCORE(
    FAST(0, SAME, "00", INOUT("12345678","12345679")),
    FAST(0, SAME, "01", INOUT("12345678","12345679")),
    FAST(0, SAME, "02", INOUT("12345678","12345679")),
    FAST(0, SAME, "03", INOUT("12345678","12345679")),
    FAST(0, SAME, "04", "12345678"),
    FAST(0, SAME, "05", "12345678"),

    FASTREQ(1, SAME, "apply t1, run action1, apply [IngressDeparserImpl]", "00", INOUT("12345678", "12345679")),
    FASTREQ(1, SAME, "miss t1, hit t1", "01", INOUT("12345678", "12345679")),
    FASTREQ(1, SAME, "not hit t1", "02", INOUT("12345678", "12345679")),
    FAST(1, SAME, "03", INOUT("12345678", "12345679")),
    FAST(1, SAME, "04", "12345678"),
    FAST(1, SAME, "05", "12345678")
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test", &t4p4s_testcase_test, "v1model" },
    { "psa",  &t4p4s_testcase_test, "psa" },
    TEST_SUITE_END,
};
