// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        FAST(0, 12345, "00", INOUT("12345678","12345679")),
        FAST(0, 12345, "01", INOUT("12345678","12345679")),
        FAST(0, 12345, "02", INOUT("12345678","12345679")),
        FAST(0, 12345, "03", INOUT("12345678","12345679")),
        FAST(0, 12345, "04", INOUT("12345678","12345679")),
        FAST(0, 12345, INOUT("05", "04"), INOUT("12345678","12345679")),

        FASTREQ(1, 12345, "apply t1, run action1, apply [IngressDeparserImpl]", "00", INOUT("12345678", "12345679")),
        FASTREQ(1, 12345, "miss t1, hit t1", "01", INOUT("12345678", "12345679")),
        FASTREQ(1, 12345, "not hit t1", "02", INOUT("12345678", "12345679")),
        FAST(1, 12345, "03", INOUT("12345678", "12345679")),
        FAST(1, 12345, "04", INOUT("12345678", "12345679")),
        FAST(1, 12345, "05", INOUT("12345678", "12345679")),

        FEND,
    },
    {
        FEND,
    },
};

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test, "psa" },
    TEST_SUITE_END,
};
