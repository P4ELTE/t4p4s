// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = SINGLE_LCORE(
    FAST(0, 0, "00", INOUT("12345678","12345679")),
    FAST(0, 0, "01", INOUT("12345678","12345679")),
    FAST(0, 0, "02", INOUT("12345678","12345679")),
    FAST(0, 0, "03", INOUT("12345678","12345679")),
    FAST(0, 0, "04", INOUT("12345678","12345679")),
    FAST(0, 0, INOUT("05", "04"), INOUT("12345678","12345679")),

    FASTREQ(1, 0, "apply t1, run action1, apply [IngressDeparserImpl]", "00", INOUT("12345678", "12345679")),
    FASTREQ(1, 0, "miss t1, hit t1", "01", INOUT("12345678", "12345679")),
    FASTREQ(1, 0, "not hit t1", "02", INOUT("12345678", "12345679")),
    FAST(1, 0, "03", INOUT("12345678", "12345679")),
    FAST(1, 0, "04", INOUT("12345678", "12345679")),
    FAST(1, 0, "05", INOUT("12345678", "12345679"))
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test", &t4p4s_testcase_test, "v1model" },
    { "psa",  &t4p4s_testcase_test, "psa" },
    TEST_SUITE_END,
};
