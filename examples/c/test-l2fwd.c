// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] =
    SINGLE_LCORE(
        WAIT_FOR_CTL,
        SLOWREQ(1, 11, "hit smac, hit dmac", hETH4(ETH01, ETH1A)),
        FASTREQ(1, 22, "hit smac, hit dmac", hETH4(ETH02, ETH1A)),
        FASTREQ(1, 33, "hit smac, hit dmac", hETH4(ETH03, ETH1A)),
        FASTREQ(1, 44, "hit smac, hit dmac", hETH4(ETH04, ETH1A)),
        
        FAST(1, BCAST, hETH4("010101010101", "101010101010"))
    );

fake_cmd_t t4p4s_testcase_bcast[][RTE_MAX_LCORE] =
    SINGLE_LCORE(
        SLOWREQ(1, BCAST, "hit smac, hit dmac", hETH4("AAAAAAAAAAAA", "BBBBBBBBBBBB")),
        FASTREQ(1, BCAST, "hit smac, hit dmac", hETH4("AAAAAAAAAAAB", "BBBBBBBBBBBC")),
        FASTREQ(1, BCAST, "hit smac, hit dmac", hETH4("AAAAAAAAAAAC", "BBBBBBBBBBBD"))
    );


fake_cmd_t t4p4s_testcase_test2[][RTE_MAX_LCORE] =
    LCORES(
        LCORE(
            WAIT_FOR_CTL,
            SLOW(1, BCAST, hETH4(ETH1A, ETH1B))
        ),
        LCORE(
            WAIT_FOR_CTL,
            WAIT_FOR_CTL, // ctl replies to lcore#1
            FAST(1, 1, hETH4(ETH1B, ETH02))
        )
    );


fake_cmd_t t4p4s_testcase_payload[][RTE_MAX_LCORE] =
    LCORES(
        LCORE(
            FAST(0, BCAST, hETH4(ETH1A, ETH01) PAYLOAD01),
            FAST(0, BCAST, hETH4(ETH1A, ETH02) PAYLOAD02),
            SLOW(0,    11, hETH4(ETH01, ETH1A) PAYLOAD03),
            FAST(0,    22, hETH4(ETH02, ETH1A) PAYLOAD03)
        ),
        LCORE(
            FAST(0, BCAST, hETH4(ETH1A, ETH03) PAYLOAD01),
            FAST(0, BCAST, hETH4(ETH1A, ETH04) PAYLOAD02),
            SLOW(0,    33, hETH4(ETH03, ETH1A) PAYLOAD03),
            FAST(0,    44, hETH4(ETH04, ETH1A) PAYLOAD03)
        )
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "bcast",          &t4p4s_testcase_bcast, "v1model" },
    { "test",           &t4p4s_testcase_test, "v1model" },
    { "test2",          &t4p4s_testcase_test2, "v1model" },
    { "payload",        &t4p4s_testcase_payload, "v1model" },
    TEST_SUITE_END,
};
