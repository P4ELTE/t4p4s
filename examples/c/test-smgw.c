// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_common[][RTE_MAX_LCORE] = SINGLE_LCORE(
    FAST(0, BCAST, hETH4(INOUT(ETH04, ETH1A), INOUT(ETH1A, ETH04)), hIP4("00", cIP4_0, cIP4_0))
    );


testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test", &t4p4s_testcase_common, "v1model" },
    { "psa",  &t4p4s_testcase_common, "psa" },
    TEST_SUITE_END,
};
