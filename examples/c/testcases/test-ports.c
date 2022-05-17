// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_common[][RTE_MAX_LCORE] = SINGLE_LCORE(
    FAST(0,   SAME, INOUT("ffffffff", "00000000"), INOUT("ffffffff", "00000000")),
    FAST(1,   SAME, INOUT("ffffffff", "00000001"), INOUT("ffffffff", "00000001")),
    FAST(511, SAME, INOUT("ffffffff", "000001ff"), INOUT("ffffffff", "000001ff"))
    );

fake_cmd_t t4p4s_testcase_psa[][RTE_MAX_LCORE] = SINGLE_LCORE(
    FAST(512,        SAME, INOUT("ffffffff", "00000200"), INOUT("ffffffff", "00000200")),
    FAST(0x7EDCBA98, SAME, INOUT("00000000", "7edcba98"), INOUT("00000000", "7edcba98"))
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test", &t4p4s_testcase_common, "v1model" },
    { "psa",  &t4p4s_testcase_common, "psa" },
    { "psa32b",  &t4p4s_testcase_psa, "psa" },
    TEST_SUITE_END,
};
