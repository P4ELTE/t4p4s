// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        FSLEEP(200),
        {FAKE_PKT, 0, 1, ETH(ETH01, ETH1A, "0000000000000000000000000000000000000000"), 200, 11, ETH(ETH01, ETH1A, "0000000000000000000000000000000000000000")},
        {FAKE_PKT, 0, 1, ETH(ETH02, ETH1A, "0000000000000000000000000000000000000000"), 200, 22, ETH(ETH02, ETH1A, "0000000000000000000000000000000000000000")},
        {FAKE_PKT, 0, 1, ETH(ETH03, ETH1A, "0000000000000000000000000000000000000000"), 200, 33, ETH(ETH03, ETH1A, "0000000000000000000000000000000000000000")},
        {FAKE_PKT, 0, 1, ETH(ETH04, ETH1A, "0000000000000000000000000000000000000000"), 200, 44, ETH(ETH04, ETH1A, "0000000000000000000000000000000000000000")},
        FEND,
    },
    {
        FEND,
    },
};

fake_cmd_t t4p4s_testcase_ipv4[][RTE_MAX_LCORE] = {
    {
        FSLEEP(200),
        {FAKE_PKT, 0, 1, IPV4(ETH01, "00000000", ETH1A, "00000000"), 0, NO_OUTPUT},
        FEND,
    },
    {
        FEND,
    },
};

fake_cmd_t t4p4s_testcase_arp[][RTE_MAX_LCORE] = {
    {
        FSLEEP(200),
        {FAKE_PKT, 0, 1, ARP(ETH01, ETH1A), 0, NO_OUTPUT},
        FEND,
    },
    {
        FEND,
    },
};

fake_cmd_t t4p4s_testcase_arp_ipv4[][RTE_MAX_LCORE] = {
    {
        FSLEEP(200),
        {FAKE_PKT, 0, 1, ARP_IPV4(ETH01, ETH1A, "0000000000000000000000000000000000000000"), 0, NO_OUTPUT},
        FEND,
    },
    {
        FEND,
    },
};

fake_cmd_t t4p4s_testcase_icmp[][RTE_MAX_LCORE] = {
    {
        FSLEEP(200),
        {FAKE_PKT, 0, 1, ICMP(ETH01, "00000000", ETH1A, "00000000", "00000000"), 0, NO_OUTPUT},
        FEND,
    },
    {
        FEND,
    },
};

fake_cmd_t t4p4s_testcase_gtp[][RTE_MAX_LCORE] = {
    {
        FSLEEP(200),
        {FAKE_PKT, 0, 1, GTP(ETH01, "00000000", ETH1A, "00000000", "00000000"), 0, NO_OUTPUT},
        FEND,
    },
    {
        FEND,
    },
};

fake_cmd_t t4p4s_testcase_udp[][RTE_MAX_LCORE] = {
    {
        FSLEEP(200),
        {FAKE_PKT, 0, 1, UDP(ETH01, "00000000", "0000", ETH1A, "00000000", "0000"), 0, NO_OUTPUT},
        FEND,
    },
    {
        FEND,
    },
};

fake_cmd_t t4p4s_testcase_gtpv1[][RTE_MAX_LCORE] = {
    {
        FSLEEP(200),
        {FAKE_PKT, 0, 1, GTPv1(ETH01, "00000000",  ETH1A, "00000000", 0), 0, NO_OUTPUT},
        {FAKE_PKT, 0, 1, GTPv1(ETH01, "00000000",  ETH1A, "00000000", 1), 0, NO_OUTPUT},
        FEND,
    },
    {
        FEND,
    },
};

fake_cmd_t t4p4s_testcase_gtpv2[][RTE_MAX_LCORE] = {
    {
        FSLEEP(200),
        {FAKE_PKT, 0, 1, GTPv2(ETH01, "00000000", ETH1A, "00000000", 0), 0, NO_OUTPUT},
        {FAKE_PKT, 0, 1, GTPv2(ETH01, "00000000", ETH1A, "00000000", 1), 0, NO_OUTPUT},
        FEND,
    },
    {
        FEND,
    },
};

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test },
    { "ipv4",           &t4p4s_testcase_ipv4 },
    { "icmp",           &t4p4s_testcase_icmp },
    { "gtp",            &t4p4s_testcase_gtp },
    { "gtpv1",          &t4p4s_testcase_gtpv1 },
    { "gtpv2",          &t4p4s_testcase_gtpv2 },
    { "arp",            &t4p4s_testcase_arp },
    { "arp_ipv4",       &t4p4s_testcase_arp_ipv4 },
    { "udp",            &t4p4s_testcase_udp },
    TEST_SUITE_END,
};
