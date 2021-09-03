// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        WAIT_FOR_CTL,
        SLOW(1, 11, hETH4(ETH01, ETH1A), REQ("hit smac, hit dmac")),
        FAST(1, 22, hETH4(ETH02, ETH1A), REQ("hit smac, hit dmac")),
        FAST(1, 33, hETH4(ETH03, ETH1A), REQ("hit smac, hit dmac")),
        FAST(1, 44, hETH4(ETH04, ETH1A), REQ("hit smac, hit dmac")),

        // TODO
        // FAST(1, 44, hETH4(ETH04, ETH1A) REMOVED("1234") ADDED("5678")),

        FEND,
    },
    {
        FEND,
    },
};

fake_cmd_t t4p4s_testcase_bcast[][RTE_MAX_LCORE] = {
    {
        SLOW(1, BCAST, hETH4("AAAAAAAAAAAA", "BBBBBBBBBBBB"), REQ("hit smac, hit dmac")),
        FAST(1, BCAST, hETH4("AAAAAAAAAAAB", "BBBBBBBBBBBC"), REQ("hit smac, hit dmac")),
        FAST(1, BCAST, hETH4("AAAAAAAAAAAC", "BBBBBBBBBBBD"), REQ("hit smac, hit dmac")),
        FEND,
    },
    {
        FEND,
    },
};


fake_cmd_t t4p4s_testcase_test2[][RTE_MAX_LCORE] = {
    {
        WAIT_FOR_CTL,
        SLOW(1, BCAST, hETH4(ETH1A, ETH1B)),
        FEND,
    },
    {
        WAIT_FOR_CTL,
        WAIT_FOR_CTL, // ctl replies to lcore#1
        FSLEEP(200),
        FAST(1, 1, hETH4(ETH1B, ETH02)),
        FEND,
    },
};


fake_cmd_t t4p4s_testcase_payload[][RTE_MAX_LCORE] = {
    {
        FSLEEP(200),
        {FAKE_PKT, 0, 0, ETH(ETH1A, ETH01, PAYLOAD01), NO_CTL_REPLY, T4P4S_BROADCAST_PORT, ETH(ETH1A, ETH01, PAYLOAD01)},
        {FAKE_PKT, 0, 0, ETH(ETH1A, ETH02, PAYLOAD02), NO_CTL_REPLY, T4P4S_BROADCAST_PORT, ETH(ETH1A, ETH02, PAYLOAD02)},
        {FAKE_PKT, 0, 0, ETH(ETH01, ETH1A, PAYLOAD03), NO_CTL_REPLY,  11, ETH(ETH01, ETH1A, PAYLOAD03)},
        {FAKE_PKT, 0, 0, ETH(ETH02, ETH1A, PAYLOAD04), NO_CTL_REPLY,  22, ETH(ETH02, ETH1A, PAYLOAD04)},

        FEND,
    },

    {
        FSLEEP(200),
        {FAKE_PKT, 0, 1, ETH(ETH1A, ETH03, PAYLOAD11), NO_CTL_REPLY, T4P4S_BROADCAST_PORT, ETH(ETH1A, ETH03, PAYLOAD11)},
        {FAKE_PKT, 0, 1, ETH(ETH1A, ETH04, PAYLOAD12), NO_CTL_REPLY, T4P4S_BROADCAST_PORT, ETH(ETH1A, ETH04, PAYLOAD12)},
        {FAKE_PKT, 0, 1, ETH(ETH03, ETH1A, PAYLOAD13), NO_CTL_REPLY,  33, ETH(ETH03, ETH1A, PAYLOAD13)},
        {FAKE_PKT, 0, 1, ETH(ETH04, ETH1A, PAYLOAD14), NO_CTL_REPLY,  44, ETH(ETH04, ETH1A, PAYLOAD14)},

        FEND,
    },
};

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "bcast",          &t4p4s_testcase_bcast },
    { "test",           &t4p4s_testcase_test },
    { "test2",          &t4p4s_testcase_test2 },
    { "payload",        &t4p4s_testcase_payload },
    TEST_SUITE_END,
};
