// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        FAST(0, 0, "00", "68656c6c6f776f726c64617364313233", OUT("3d770bc1f907b2d3315edf7129b2245b")),
        
        FAST(0, 0, "00", "68656c6c6f776f726c64617364", OUT("000000"), OUT("38886a7333ce4e56aee3731e115b7343")),
        
        FAST(0, 0, "00", "68656c6c6f776f726c64", OUT("000000000000"), OUT("a3a7d3996f5c23ce404c3588a182314f")),
        
        FAST(0, 0, "00", "68656c6c6f", OUT("0000000000000000000000"), OUT("fac7c5ceca254536550cdd5271448a49")),
        
        FAST(0, 0, "01", "68656c6c6f776f726c64617364313233", OUT("00"), OUT("4181d3eb5ec1b6e2873c29d5eeeccbfa")),
        
        FAST(0, 0, "01", "68656c6c6f776f726c64617364", OUT("00000000"), OUT("67c11c0e82197607ba6a5a6e92a11a75")),
        
        FAST(0, 0, "01", "68656c6c6f776f726c64", OUT("00000000000000"), OUT("14349a8d4a8d27f374c3a5b5f38f3b95")),
        
        FAST(0, 0, "01", "68656c6c6f", OUT("000000000000000000000000"), OUT("a0c850de60ff6b5e4e27a41c7afd9c63")),

        FAST(0, 0, "03", "aaaaaa68656c6c6f", OUT("0000000000000000000000"), OUT("fac7c5ceca254536550cdd5271448a49")),
        
        FAST(0, 0, "05", "bbbbaaaaaa68656c6c6f", OUT("0000000000000000000000"), OUT("fac7c5ceca254536550cdd5271448a49")),
        
        FAST(0, 0, "06", "ccbbbbaaaaaa68656c6c6f", OUT("0000000000000000000000"), OUT("fac7c5ceca254536550cdd5271448a49")),
        
        FAST(0, 0, "0a", "ddddddddccbbbbaaaaaa68656c6c6f", OUT("0000000000000000000000"), OUT("fac7c5ceca254536550cdd5271448a49")),
        
        FEND,
    },
    {
        FEND,
    },
};

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test, "v1model" },
    TEST_SUITE_END,
};
