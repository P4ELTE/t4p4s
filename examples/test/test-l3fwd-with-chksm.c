// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        FSLEEP(INIT_WAIT_CONTROLPLANE_LONG_MILLIS),
        UNKNOWN_PKT(LPM_ETH1, LPM_ETH2, "000000000000000000000000" LPM1_TOP16B "0102" LPM2_TOP16B "0304"),
        // LEARNED_PKT(0, "dddddddd0000", LPM1 "0037"),
        // LEARNED_PKT(3, "dddddddd0000", LPM1 "00ab"),
        // LEARNED_PKT(9, "dddddddd0000", LPM1 "00ff"),
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
