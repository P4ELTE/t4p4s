// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        {FAKE_PKT, 0, 1,    FDATA("00","68656c6c6f776f726c64617364313233"),NO_CTL_REPLY, 0,
                            FDATA("00","68656c6c6f776f726c64617364313233","3d770bc1f907b2d3315edf7129b2245b")},
        {FAKE_PKT, 0, 1,    FDATA("00","68656c6c6f776f726c64617364"), NO_CTL_REPLY, 0,
                            FDATA("00","68656c6c6f776f726c64617364000000","38886a7333ce4e56aee3731e115b7343")},
        {FAKE_PKT, 0, 1,    FDATA("00","68656c6c6f776f726c64"), NO_CTL_REPLY, 0,
                            FDATA("00","68656c6c6f776f726c64000000000000","a3a7d3996f5c23ce404c3588a182314f")},
        {FAKE_PKT, 0, 1,    FDATA("00","68656c6c6f"), NO_CTL_REPLY, 0,
                            FDATA("00","68656c6c6f0000000000000000000000","fac7c5ceca254536550cdd5271448a49")},
        {FAKE_PKT, 0, 1,    FDATA("01","68656c6c6f776f726c64617364313233"), NO_CTL_REPLY, 0,
                            FDATA("01","68656c6c6f776f726c6461736431323300","4181d3eb5ec1b6e2873c29d5eeeccbfa")},
        {FAKE_PKT, 0, 1,    FDATA("01","68656c6c6f776f726c64617364"), NO_CTL_REPLY, 0,
                            FDATA("01","68656c6c6f776f726c6461736400000000","67c11c0e82197607ba6a5a6e92a11a75")},
        {FAKE_PKT, 0, 1,    FDATA("01","68656c6c6f776f726c64"), NO_CTL_REPLY, 0,
                            FDATA("01","68656c6c6f776f726c6400000000000000","14349a8d4a8d27f374c3a5b5f38f3b95")},
        {FAKE_PKT, 0, 1,    FDATA("01","68656c6c6f"), NO_CTL_REPLY, 0,
                            FDATA("01","68656c6c6f000000000000000000000000","a0c850de60ff6b5e4e27a41c7afd9c63")},

        {FAKE_PKT, 0, 1,    FDATA("03","aaaaaa68656c6c6f"), NO_CTL_REPLY, 0,
                            FDATA("03","aaaaaa68656c6c6f0000000000000000000000","fac7c5ceca254536550cdd5271448a49")},
        {FAKE_PKT, 0, 1,    FDATA("05","bbbbaaaaaa68656c6c6f"), NO_CTL_REPLY, 0,
                            FDATA("05","bbbbaaaaaa68656c6c6f0000000000000000000000","fac7c5ceca254536550cdd5271448a49")},
        {FAKE_PKT, 0, 1,    FDATA("06","ccbbbbaaaaaa68656c6c6f"), NO_CTL_REPLY, 0,
                            FDATA("06","ccbbbbaaaaaa68656c6c6f0000000000000000000000","fac7c5ceca254536550cdd5271448a49")},
        {FAKE_PKT, 0, 1,    FDATA("0a","ddddddddccbbbbaaaaaa68656c6c6f"), NO_CTL_REPLY, 0,
                            FDATA("0a","ddddddddccbbbbaaaaaa68656c6c6f0000000000000000000000","fac7c5ceca254536550cdd5271448a49")},
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
