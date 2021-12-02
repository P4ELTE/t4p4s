// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"
#include "gen_defs.h"

#ifndef T4P4S_TESTENTRY
    #define T4P4S_TESTENTRY 0
#endif

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = SINGLE_LCORE(
    WAIT_FOR_CTL,
    #if T4P4S_TESTENTRY == __LINE__ || T4P4S_TESTENTRY == 0
        SLOW(0, 11, hETH4(ETH01, ETH1A), hIP4_0),
    #endif

    #if T4P4S_TESTENTRY == __LINE__ || T4P4S_TESTENTRY == 0
        SLOW(0, 22, hETH4(ETH02, ETH1A), hIP4_0),
    #endif

    #if T4P4S_TESTENTRY == __LINE__ || T4P4S_TESTENTRY == 0
        SLOW(0, 33, hETH4(ETH03, ETH1A), hIP4_0),
    #endif

    #if T4P4S_TESTENTRY == __LINE__ || T4P4S_TESTENTRY == 0
        SLOW(0, 44, hETH4(ETH04, ETH1A), hIP4_0),
    #endif
    FEND);

fake_cmd_t t4p4s_testcase_ipv4[][RTE_MAX_LCORE] = SINGLE_LCORE(
    WAIT_FOR_CTL,
    #if T4P4S_TESTENTRY == __LINE__ || T4P4S_TESTENTRY == 0
        FAST(0, DROP, hETH4(ETH01, ETH1A), hIP4_0),
    #endif
    FEND);

fake_cmd_t t4p4s_testcase_arp[][RTE_MAX_LCORE] = SINGLE_LCORE(
    WAIT_FOR_CTL,
    #if T4P4S_TESTENTRY == __LINE__ || T4P4S_TESTENTRY == 0
        FAST(0, DROP, hETH4(ETH01, ETH1A), hMISC("ARP", c0x16B)),
    #endif
    FEND);

fake_cmd_t t4p4s_testcase_arp_ipv4[][RTE_MAX_LCORE] = SINGLE_LCORE(
    WAIT_FOR_CTL,
    #if T4P4S_TESTENTRY == __LINE__ || T4P4S_TESTENTRY == 0
        FAST(0, BCAST, hETH4(ETH01, ETH1A), hMISC("ARP_IPV4", "0000", c0x12B, cIP4_0, c0x12B, cIP4_0)),
    #endif
    FEND);

fake_cmd_t t4p4s_testcase_icmp[][RTE_MAX_LCORE] = SINGLE_LCORE(
    WAIT_FOR_CTL,
    #if T4P4S_TESTENTRY == __LINE__ || T4P4S_TESTENTRY == 0
        FAST(0, DROP, hETH4(ETH01, ETH1A), hIP4_0, hMISC("ICMP", cIP4_0)),
    #endif
    FEND);

fake_cmd_t t4p4s_testcase_udp[][RTE_MAX_LCORE] = SINGLE_LCORE(
    WAIT_FOR_CTL,
    #if T4P4S_TESTENTRY == __LINE__ || T4P4S_TESTENTRY == 0
        // FAST(0, DROP, UDP(ETH01, cIP4_0, "0000", ETH1A, cIP4_0, "0000")),
    #endif
    FEND);

fake_cmd_t t4p4s_testcase_gtp[][RTE_MAX_LCORE] = SINGLE_LCORE(
    WAIT_FOR_CTL,
    #if T4P4S_TESTENTRY == __LINE__ || T4P4S_TESTENTRY == 0
        // FAST(0, DROP, GTP(ETH01, cIP4_0, ETH1A, "60", "00", "0000")),
    #endif
    FEND);

fake_cmd_t t4p4s_testcase_gtpv1[][RTE_MAX_LCORE] = SINGLE_LCORE(
    WAIT_FOR_CTL,
    #if T4P4S_TESTENTRY == __LINE__ || T4P4S_TESTENTRY == 0
        // FAST(0, DROP, GTPv1(ETH01, ETH1A, cIP4_0, 0)),
    #endif

    #if T4P4S_TESTENTRY == __LINE__ || T4P4S_TESTENTRY == 0
        // FAST(0, DROP, GTPv1(ETH01, ETH1A, cIP4_0, 1)),
    #endif
    FEND);

fake_cmd_t t4p4s_testcase_gtpv2[][RTE_MAX_LCORE] = SINGLE_LCORE(
    #if T4P4S_TESTENTRY == __LINE__ || T4P4S_TESTENTRY == 0
        FASTREQ(0, BCAST, "hit dmac, hit smac", hETH4(ETH01, ETH1A), hIP4(cUDP, cIP4_0, cIP4_0), hUDP(PORT0, pGTPU, LEN0, CHKSM0), hGTP("40", "00", LEN0), hGTPv2("000000", "00")),
    #endif
    FEND);

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test, "v1model" },
    { "ipv4",           &t4p4s_testcase_ipv4, "v1model" },
    { "icmp",           &t4p4s_testcase_icmp, "v1model" },
    { "gtp",            &t4p4s_testcase_gtp, "v1model" },
    { "gtpv1",          &t4p4s_testcase_gtpv1, "v1model" },
    { "gtpv2",          &t4p4s_testcase_gtpv2, "v1model" },
    { "arp",            &t4p4s_testcase_arp, "v1model" },
    { "arp_ipv4",       &t4p4s_testcase_arp_ipv4, "v1model" },
    { "udp",            &t4p4s_testcase_udp, "v1model" },
    TEST_SUITE_END,
};

