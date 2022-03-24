// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

#define c0toF   "0123456789abcdef"
#define cABCDx3 "abcdabcdabcd"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = SINGLE_LCORE(
    FAST(0, 0, IN("00000001"), INOUT(             "00", x3("00"))),
    FAST(0, 0, IN("00000001"), INOUT(             "01", x3("01"))),
    FAST(0, 0, IN("00000001"), INOUT(             "ab", x3("ab"))),
    FAST(0, 0, IN("00000002"), INOUT(           "abcd", x3("abcd"))),
    FAST(0, 0, IN("00000008"), INOUT(            c0toF, x3(c0toF))),
    FAST(0, 0, IN("0000000c"), INOUT(c0toF c0x4B, x3(c0toF c0x4B))),

    // Note: this is longer than the maximum varbit length of 12
    #define OVERSIZED_CONTENT c0x13B
    FAST(0, DROP, IN("0000000d"), IN(OVERSIZED_CONTENT))
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test", &t4p4s_testcase_test, "v1model" },
    { "psa",  &t4p4s_testcase_test, "psa" },
    TEST_SUITE_END,
};
