// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = SINGLE_LCORE(
    FAST(0, 0, INOUT("0068656c6c6f776f726c64617364313233", "3d770bc1f907b2d3315edf7129b2245b")),
    FAST(0, 0, INOUT("0068656c6c6f776f726c64617364", "38886a7333ce4e56aee3731e115b7343")),
    FAST(0, 0, INOUT("0068656c6c6f776f726c64", "a3a7d3996f5c23ce404c3588a182314f")),
    FAST(0, 0, INOUT("0068656c6c6f", "fac7c5ceca254536550cdd5271448a49")),
    FAST(0, 0, INOUT("0168656c6c6f776f726c64617364313233", "4181d3eb5ec1b6e2873c29d5eeeccbfa")),
    FAST(0, 0, INOUT("0168656c6c6f776f726c64617364", "67c11c0e82197607ba6a5a6e92a11a75")),
    FAST(0, 0, INOUT("0168656c6c6f776f726c64", "14349a8d4a8d27f374c3a5b5f38f3b95")),
    FAST(0, 0, INOUT("0168656c6c6f", "a0c850de60ff6b5e4e27a41c7afd9c63")),

    FAST(0, 0, INOUT("03aaaaaa68656c6c6f", "fac7c5ceca254536550cdd5271448a49")),
    FAST(0, 0, INOUT("05bbbbaaaaaa68656c6c6f", "fac7c5ceca254536550cdd5271448a49")),
    FAST(0, 0, INOUT("06ccbbbbaaaaaa68656c6c6f", "fac7c5ceca254536550cdd5271448a49")),
    FAST(0, 0, INOUT("0addddddddccbbbbaaaaaa68656c6c6f", "fac7c5ceca254536550cdd5271448a49"))
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test, "v1model" },
    TEST_SUITE_END,
};
