// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = 
	SINGLE_LCORE(
        WAIT_FOR_CTL,
        SLOWREQ(1, 11, "miss smac, hit dmac", hETH4(ETH01, ETH1A)),
        FASTREQ(1, 22, "hit smac, hit dmac", hETH4(ETH02, ETH1A)),
        FASTREQ(1, 33, "hit smac, hit dmac", hETH4(ETH03, ETH1A)),
        FASTREQ(1, 44, "hit smac, hit dmac", hETH4(ETH04, ETH1A)),

        FASTREQ(1, BCAST, "miss smac, miss dmac", hETH4(x6("01"), x6("10")))
    );

fake_cmd_t t4p4s_testcase_bcast[][RTE_MAX_LCORE] = SINGLE_LCORE(
    SLOW(0, BCAST, hETH4("AAAAAAAAAAAA", "BBBBBBBBBBBB"))
    );

fake_cmd_t t4p4s_testcase_test2[][RTE_MAX_LCORE] = LCORES(
    LCORE(
        WAIT_FOR_CTL,
        SLOW(0, BCAST, hETH4(ETH1A, ETH1B))
        ),
    LCORE(
        WAIT_FOR_CTL,
        WAIT_FOR_CTL, // process the packet of lcore#1
        SLOW(0, 3, hETH4(ETH1B, ETH02))
        )
    );

fake_cmd_t t4p4s_testcase_payload[][RTE_MAX_LCORE] = LCORES(
    LCORE(
        WAIT_FOR_CTL,
        SLOW(0, BCAST, hETH4(ETH1A, ETH01), PAYLOAD01),
        SLOW(0, BCAST, hETH4(ETH1A, ETH02), PAYLOAD02),
        FAST(0, 11, hETH4(ETH01, ETH1A), PAYLOAD03),
        FAST(0, 22, hETH4(ETH02, ETH1A), PAYLOAD04)
        ),
    LCORE(
        WAIT_FOR_CTL,
        SLOW(0, BCAST, hETH4(ETH1A, ETH03), PAYLOAD11),
        SLOW(0, BCAST, hETH4(ETH1A, ETH04), PAYLOAD12),
        FAST(0, 33, hETH4(ETH03, ETH1A), PAYLOAD13),
        FAST(0, 44, hETH4(ETH04, ETH1A), PAYLOAD14)
        )
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "bcast",          &t4p4s_testcase_bcast, "v1model" },
    { "test",           &t4p4s_testcase_test, "v1model" },
    { "test2",          &t4p4s_testcase_test2, "v1model" },
    { "payload",        &t4p4s_testcase_payload, "v1model" },

    { "psa-bcast",      &t4p4s_testcase_bcast, "psa" },
    { "psa",            &t4p4s_testcase_test, "psa" },
    { "psa-test2",      &t4p4s_testcase_test2, "psa" },
    { "psa-payload",    &t4p4s_testcase_payload, "psa" },
    TEST_SUITE_END,
};
