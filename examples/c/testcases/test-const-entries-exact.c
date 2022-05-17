// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = SINGLE_LCORE(
    FAST(0, SAME, IN(hETH4(ETH04, ETH1A)), OUT("01"), OUT("AA")),

    FAST(0, SAME, IN(hETH4("000012345678", "00009ABCDEF0")), OUT("AA"), OUT("01")),
    FAST(0, SAME, IN(hETH4("00000D15EA5E", "0000DEADBEEF")), OUT("AA"), OUT("02")),
    FAST(0, SAME, IN(hETH4("0000DEAD10CC", "0000BAAAAAAD")), OUT("AA"), OUT("03")),
    FAST(0, SAME, IN(hETH4("000000000064", "0000000000C8")), OUT("AA"), OUT("04")),
    FAST(0, SAME, IN(hETH4("00000000012C", "000000000190")), OUT("AA"), OUT("05"))

    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test", &t4p4s_testcase_test, "v1model" },
    { "psa",  &t4p4s_testcase_test, "psa" },
    TEST_SUITE_END,
};
