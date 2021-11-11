// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        FAST(0, 0, INOUT("0000000100", "000000")),
        FAST(0, 0, INOUT("0000000101", "010101")),
        FAST(0, 0, INOUT("00000001ab", "ababab")),
        FAST(0, 0, INOUT("00000002abcd", "abcdabcdabcd")),
        FAST(0, 0, INOUT("00000002abcd", "abcdabcdabcd")),
        FAST(0, 0, INOUT("000000080123456789abcdef", "0123456789abcdef0123456789abcdef0123456789abcdef")),
        FAST(0, 0, INOUT("000000160123456789abcdef0123456789abcdef000000000000", "0123456789abcdef0123456789abcdef0000000000000123456789abcdef0123456789abcdef0000000000000123456789abcdef0123456789abcdef000000000000")),
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
