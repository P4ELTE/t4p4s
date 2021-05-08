// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        {FAKE_PKT,     0, 1, FDATA("0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "00000000", "00000000", "00000000", "00000000", "00000000"),
         NO_CTL_REPLY,    0, FDATA("1234", "1234", "1234", "1234", "1234", "1234", "4434", "444C", "de4c", "def0", "00001234", "00123456", "12345678", "12345678", "12345678")},
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
