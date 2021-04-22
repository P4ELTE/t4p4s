// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

#ifdef TEST_CONST_ENTRIES
    #define ENABLED_BITS TEST_CONST_ENTRIES
#else
    #define ENABLED_BITS 0xFFFFFFFF
#endif

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        {FAKE_PKT, 0, 1, FDATA("ffffffff"), NO_CTL_REPLY, 12345, {
            #if (ENABLED_BITS & (1 << 0)) != 0
                "01",
            #endif
            #if (ENABLED_BITS & (1 << 1)) != 0
                "03",
            #endif
            #if (ENABLED_BITS & (1 << 2)) != 0
                "07",
            #endif
            #if (ENABLED_BITS & (1 << 3)) != 0
                "0f",
            #endif
            #if (ENABLED_BITS & (1 << 4)) != 0
                "1f",
            #endif
            #if (ENABLED_BITS & (1 << 5)) != 0
                "3f",
            #endif
            #if (ENABLED_BITS & (1 << 6)) != 0
                "7f",
            #endif
            #if (ENABLED_BITS & (1 << 7)) != 0
                "ff",
            #endif
            #if (ENABLED_BITS & (1 << 8)) != 0
                "01ff",
            #endif
            #if (ENABLED_BITS & (1 << 9)) != 0
                "03ff",
            #endif
            #if (ENABLED_BITS & (1 << 10)) != 0
                "07ff",
            #endif
            #if (ENABLED_BITS & (1 << 11)) != 0
                "0fff",
            #endif
            #if (ENABLED_BITS & (1 << 12)) != 0
                "1fff",
            #endif
            #if (ENABLED_BITS & (1 << 13)) != 0
                "3fff",
            #endif
            #if (ENABLED_BITS & (1 << 14)) != 0
                "7fff",
            #endif
            #if (ENABLED_BITS & (1 << 15)) != 0
                "ffff",
            #endif
            #if (ENABLED_BITS & (1 << 16)) != 0
                "0001ffff",
            #endif
            #if (ENABLED_BITS & (1 << 17)) != 0
                "0003ffff",
            #endif
            #if (ENABLED_BITS & (1 << 18)) != 0
                "0007ffff",
            #endif
            #if (ENABLED_BITS & (1 << 19)) != 0
                "000fffff",
            #endif
            #if (ENABLED_BITS & (1 << 20)) != 0
                "001fffff",
            #endif
            #if (ENABLED_BITS & (1 << 21)) != 0
                "003fffff",
            #endif
            #if (ENABLED_BITS & (1 << 22)) != 0
                "007fffff",
            #endif
            #if (ENABLED_BITS & (1 << 23)) != 0
                "00ffffff",
            #endif
            #if (ENABLED_BITS & (1 << 24)) != 0
                "01ffffff",
            #endif
            #if (ENABLED_BITS & (1 << 25)) != 0
                "03ffffff",
            #endif
            #if (ENABLED_BITS & (1 << 26)) != 0
                "07ffffff",
            #endif
            #if (ENABLED_BITS & (1 << 27)) != 0
                "0fffffff",
            #endif
            #if (ENABLED_BITS & (1 << 28)) != 0
                "1fffffff",
            #endif
            #if (ENABLED_BITS & (1 << 29)) != 0
                "3fffffff",
            #endif
            #if (ENABLED_BITS & (1 << 30)) != 0
                "7fffffff",
            #endif
            #if (ENABLED_BITS & (1 << 31)) != 0
                "ffffffff",
            #endif
            ""}},

        // {FAKE_PKT, 0, 1, {"01234567", ""}, 200, 12345, {
        //     "00", "00", "00", "00", "00", "00", "00", "01",
        //     // TODO fix these
        //     "....", "....", "....", "....", "....", "....", "....", "0123",
        //     "........", "........", "........", "........", "........", "........", "........", "........",
        //     "........", "........", "........", "........", "........", "........", "........", "01234567",
        //     ""}},

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
