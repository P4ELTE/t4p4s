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
    FAST(0, 0, IN("00000016"), INOUT(c0toF c0toF c0x6B, x3(c0toF c0toF c0x6B)))
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test", &t4p4s_testcase_test, "v1model" },
    { "psa",  &t4p4s_testcase_test, "psa" },
    TEST_SUITE_END,
};
