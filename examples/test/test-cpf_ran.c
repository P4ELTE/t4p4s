// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        // WAIT_FOR_CTL,
        // FAST(1, 11, hETH4(ETH01, ETH1A) hIP(cUDP, IP4_0, IP4_0) hUDP(PORT0, pGTPU, LEN0, CHKSM0) hGTP("40", "00", LEN0) hGTPv2("000000", "00") hGTPopt("0000", "00", "00") hIP(cIPv4, IP4_0, IP4_0)),
        // FAST(1, 11, hETH4(ETH01, ETH1A) hIP(cICMP, IP4_0, IP4_0) hICMP("00", "00", "0000")),
        // FAST(1, 11, hETH4(ETH01, ETH1A) hIP(cUDP, IP4_0, IP4_0) hUDP(PORT0, pRLC, LEN0, CHKSM0) hMISC(RLC_STATUS, "000000") hTEID(c0x4B)),
        // FAST(1, 11, hETH4(ETH01, ETH1A) hIP(cUDP, IP4_0, IP4_0) hUDP(PORT0, pBUF, LEN0, CHKSM0) hMISC(BUFFERING, c0x7B) hIP(cIPv4, IP4_0, IP4_0) hUDP(PORT0, pRLC, LEN0, CHKSM0)),
        // FAST(1, 11, hETH4(ETH01, ETH1A) hIP(cUDP, IP4_0, IP4_0) hUDP(PORT0, pBUF, LEN0, CHKSM0)
        //             hMISC(BUFFERING, "01" c0x6B) hMISC(RLC_STATUS, "000000") hTEID(c0x4B)),
        // FAST(1, 11, hETH4(ETH01, ETH1A) hIP(cUDP, IP4_0, IP4_0) hUDP(PORT0, pBUF, LEN0, CHKSM0)
        //             hMISC(BUFFERING, "01" c0x6B) hMISC(RLC_STATUS, "123408") hTEID(c0x4B)
        //                                          hMISC(RLC_STATUS, "abcd00") hTEID(c0x4B)),
        FAST(1, 11, hETH4(ETH01, ETH1A) hIP(cUDP, IP4_0, IP4_0) hUDP(PORT0, pBUF, LEN0, CHKSM0)
                    hMISC(BUFFERING, "01" c0x6B) hMISC(RLC_STATUS, "123408")
                                                 hMISC(RLC_STATUS, "567808")
                                                 hMISC(RLC_STATUS, "abcd00") hTEID(c0x4B)),
        FEND,
    },
    {
        FEND,
    },
};

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test, "v1model" },
    TEST_SUITE_END,
};
