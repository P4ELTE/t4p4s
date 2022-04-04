// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_common[][RTE_MAX_LCORE] = SINGLE_LCORE(
    FAST(0, 0, "00", "68656c6c6f776f726c64617364313200"),
    FAST(0, 0, "01", "1168656c6c6f776f726c64617364313200"),
    FAST(0, 0, "02", "112268656c6c6f776f726c64617364313200"),
    FAST(0, 0, "03", "11223368656c6c6f776f726c64617364313200"),
    FAST(0, 0, "04", "1122334468656c6c6f776f726c64617364313200"),
    FAST(0, 0, "0b", "112233445566778899aabb68656c6c6f776f726c64617364313200"),

    FAST(0, 0, "00", "68656c6c6f0000000000000000000000"),
    FAST(0, 0, "01", "68656c6c6f776f726c6461736431323300"),
    FAST(0, 0, "01", "68656c6c6f776f726c6461736400000000"),
    FAST(0, 0, "02", "2268656c6c6f776f726c6461736400000000"),
    FAST(0, 0, "03", "332268656c6c6f776f726c6461736400000000"),
    FAST(0, 0, "05", "5544332268656c6c6f776f726c6461736400000000")
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "v1", &t4p4s_testcase_common, "v1model" },
    { "psa", &t4p4s_testcase_common, "psa" },
    TEST_SUITE_END,
};
