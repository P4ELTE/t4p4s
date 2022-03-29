// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] =
    SINGLE_LCORE(
        FAST(0, 0, "00", "68656c6c6f776f726c64617364313233", OUT("3d770bc1f907b2d3315edf7129b2245b")),
        FAST(0, 0, "00", "68656c6c6f776f726c64617364", OUT("38886a7333ce4e56aee3731e115b7343")),
        FAST(0, 0, "00", "68656c6c6f776f726c64", OUT("a3a7d3996f5c23ce404c3588a182314f")),
        FAST(0, 0, "00", "68656c6c6f", OUT("fac7c5ceca254536550cdd5271448a49")),
        FAST(0, 0, "01", "68656c6c6f776f726c64617364313233", OUT("4181d3eb5ec1b6e2873c29d5eeeccbfa")),
        FAST(0, 0, "01", "68656c6c6f776f726c64617364",  OUT("67c11c0e82197607ba6a5a6e92a11a75")),
        FAST(0, 0, "01", "68656c6c6f776f726c64",  OUT("14349a8d4a8d27f374c3a5b5f38f3b95")),
        FAST(0, 0, "01", "68656c6c6f", OUT("a0c850de60ff6b5e4e27a41c7afd9c63")),
        FAST(0, 0, "03", "aaaaaa68656c6c6f",  OUT("fac7c5ceca254536550cdd5271448a49")),
        FAST(0, 0, "05", "bbbbaaaaaa68656c6c6f", OUT("fac7c5ceca254536550cdd5271448a49")),
        FAST(0, 0, "06", "ccbbbbaaaaaa68656c6c6f", OUT("fac7c5ceca254536550cdd5271448a49")),
        FAST(0, 0, "0a", "ddddddddccbbbbaaaaaa68656c6c6f", OUT("fac7c5ceca254536550cdd5271448a49")),

        FAST(0,0, "2100","ddddddddddddeeeeeeeeeeee08004500001600010000400074e2010101010202020200000222000000010123456701234567012345670123456795ad5d83be4a49a5bf9e27bf63533ca7b1a59daaa8b167f3f2fcb7473d74ee50",
              OUT("947aa87dcd07127351adac3d8f274149"))
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test, "v1model" },
    TEST_SUITE_END,
};
