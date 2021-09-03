// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        {FAKE_PKT, 0, 1, FDATA("00000001", "00"), NO_CTL_REPLY, 0, FDATA("00", "00", "00")},
        {FAKE_PKT, 0, 1, FDATA("00000001", "01"), NO_CTL_REPLY, 0, FDATA("01", "01", "01")},
        {FAKE_PKT, 0, 1, FDATA("00000001", "ab"), NO_CTL_REPLY, 0, FDATA("ab", "ab", "ab")},
        // {FAKE_PKT, 0, 1, FDATA("00000002", "abcd"), NO_CTL_REPLY, 0, FDATA("abcd", "abcd", "abcd")},
        // {FAKE_PKT, 0, 1, FDATA("00000002", "abcd"), NO_CTL_REPLY, 0, FDATA("abcd", "abcd", "abcd")},
        // {FAKE_PKT, 0, 1, FDATA("00000008", "0123456789abcdef"), NO_CTL_REPLY, 0, FDATA("0123456789abcdef", "0123456789abcdef", "0123456789abcdef")},
        // {FAKE_PKT, 0, 1, FDATA("00000016", "fedcba9876543210"), NO_CTL_REPLY, 0, FDATA("fedcba9876543210", "fedcba9876543210", "fedcba9876543210")},
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
