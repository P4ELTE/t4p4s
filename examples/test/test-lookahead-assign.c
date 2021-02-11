// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        {FAKE_PKT, 0, 1, {"ffffffff", ""}, 200, 12345, {
            "01", "03", "07", "0f", "1f", "3f", "7f", "ff",
            "01ff", "03ff", "07ff", "0fff", "1fff", "3fff", "7fff", "ffff",
            "0001ffff", "0003ffff", "0007ffff", "000fffff", "001fffff", "003fffff", "007fffff", "00ffffff",
            "01ffffff", "03ffffff", "07ffffff", "0fffffff", "1fffffff", "3fffffff", "7fffffff", "ffffffff",
            ""}},

        {FAKE_PKT, 0, 1, {"01234567", ""}, 200, 12345, {
            "00", "00", "00", "00", "00", "00", "00", "01",
            // TODO fix these
            "....", "....", "....", "....", "....", "....", "....", "0123",
            "........", "........", "........", "........", "........", "........", "........", "........",
            "........", "........", "........", "........", "........", "........", "........", "01234567",
            ""}},

        // {FAKE_PKT, 0, 1, {"89abcdef", ""}, 200, 12345, {"..............", ""}},
        // {FAKE_PKT, 0, 1, {"76543210", ""}, 200, 12345, {"..............", ""}},
        // {FAKE_PKT, 0, 1, {"fedcba98", ""}, 200, 12345, {"..............", ""}},

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
