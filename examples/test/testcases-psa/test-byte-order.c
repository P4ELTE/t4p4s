// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        {FAKE_PKT, 0, 1, FDATA("00","12345678"), 200, 12345, FDATA("00","12345679"), REQUIRE(table_apply__t1_0)},
        {FAKE_PKT, 0, 1, FDATA("01","12345678"), 200, 12345, FDATA("01","12345679"), REQUIRE(table_apply__t1_0, table_miss__t1_0), FORBID(table_hit__t1_0)},
        {FAKE_PKT, 0, 1, FDATA("02","12345678"), 200, 12345, FDATA("02","12345679"), REQUIRE(none), FORBID(table_hit__t1_0)},
        {FAKE_PKT, 0, 1, FDATA("03","12345678"), 200, 12345, FDATA("03","12345679"), REQUIRE(none), FORBID(none)},
        {FAKE_PKT, 0, 1, FDATA("04","12345678"), 200, 12345, FDATA("04","12345679")},
        {FAKE_PKT, 0, 1, FDATA("05","12345678"), 200, 12345, FDATA("04","12345679")},
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
