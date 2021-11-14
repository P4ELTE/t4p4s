// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
        {
                FSLEEP(200),
                {FAKE_PKT, 0, 1, ETH(ETH01, ETH1A), 200, 11, ETH(ETH01, ETH1A)},
                {FAKE_PKT, 0, 1, ETH(ETH02, ETH1A), 200, 22, ETH(ETH02, ETH1A)},
                {FAKE_PKT, 0, 1, ETH(ETH03, ETH1A), 200, 33, ETH(ETH03, ETH1A)},
                {FAKE_PKT, 0, 1, ETH(ETH04, ETH1A), 200, 44, ETH(ETH04, ETH1A)},
                FEND,
        },
        {
                FEND,
        },
};

fake_cmd_t t4p4s_testcase_bcast[][RTE_MAX_LCORE] = {
        {
                {FAKE_PKT, 0, 1, ETH("AAAAAAAAAAAA", "BBBBBBBBBBBB"), 200, T4P4S_BROADCAST_PORT, ETH("AAAAAAAAAAAA", "BBBBBBBBBBBB")},
                FEND,
        },
        {
                FEND,
        },
};


fake_cmd_t t4p4s_testcase_test2[][RTE_MAX_LCORE] = {
        {
                FSLEEP(200),
                {FAKE_PKT, 0, 3, ETH(ETH1A, ETH1B), 200, T4P4S_BROADCAST_PORT, ETH(ETH1A, ETH1B)},
                FEND,
        },
        {
                FSLEEP(500),
                {FAKE_PKT, 0, 10, ETH(ETH1B, ETH02), 200, 3, ETH(ETH1B, ETH02)},
                FEND,
        },
};


fake_cmd_t t4p4s_testcase_payload[][RTE_MAX_LCORE] = {
        {
                FSLEEP(200),
                {FAKE_PKT, 0, 0, ETH(ETH1A, ETH01, PAYLOAD01), 200, T4P4S_BROADCAST_PORT, ETH(ETH1A, ETH01, PAYLOAD01)},
                {FAKE_PKT, 0, 0, ETH(ETH1A, ETH02, PAYLOAD02), 200, T4P4S_BROADCAST_PORT, ETH(ETH1A, ETH02, PAYLOAD02)},
                {FAKE_PKT, 0, 0, ETH(ETH01, ETH1A, PAYLOAD03),   0,  11, ETH(ETH01, ETH1A, PAYLOAD03)},
                {FAKE_PKT, 0, 0, ETH(ETH02, ETH1A, PAYLOAD04),   0,  22, ETH(ETH02, ETH1A, PAYLOAD04)},

                FEND,
        },

        {
                FSLEEP(200),
                {FAKE_PKT, 0, 1, ETH(ETH1A, ETH03, PAYLOAD11), 200, T4P4S_BROADCAST_PORT, ETH(ETH1A, ETH03, PAYLOAD11)},
                {FAKE_PKT, 0, 1, ETH(ETH1A, ETH04, PAYLOAD12), 200, T4P4S_BROADCAST_PORT, ETH(ETH1A, ETH04, PAYLOAD12)},
                {FAKE_PKT, 0, 1, ETH(ETH03, ETH1A, PAYLOAD13),   0,  33, ETH(ETH03, ETH1A, PAYLOAD13)},
                {FAKE_PKT, 0, 1, ETH(ETH04, ETH1A, PAYLOAD14),   0,  44, ETH(ETH04, ETH1A, PAYLOAD14)},

                FEND,
        },
};

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
        { "bcast",          &t4p4s_testcase_bcast, "v1model" },
        { "test",           &t4p4s_testcase_test, "v1model" },
        { "test2",          &t4p4s_testcase_test2, "v1model" },
        { "payload",        &t4p4s_testcase_payload, "v1model" },
        TEST_SUITE_END,
};
