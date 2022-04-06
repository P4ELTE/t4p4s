// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = SINGLE_LCORE(
        WAIT_FOR_CTL,
        
        FASTREQ(1, DROP, "miss macfwd", hETH(ETH04, ETH1A, cIPV4), 
                                        hIP4("00", cIP4_0, cIP4_0)),
                                              
        FASTREQ(1, DROP, "miss macfwd", hETH(ETH04, ETH1A, cARP), 
                                        hARP("0000"),
                                        hARP4("000000000000", "00000000", "000000000000", "00000000")),
                    
        FASTREQ(1, DROP, "hit macfwd, miss ipv4_lpm, miss nexthops", 
                         hETH(LPM_ETH2, ETH1A, cIPV4), 
                         hIP4("00", cIP4_0, cIP4_0)),
                                              
        FASTREQ(1, DROP, "hit macfwd, miss ipv4_lpm, miss nexthops", 
                         hETH(LPM_ETH2, ETH1A, cARP), 
                         hARP("0000"),
                         hARP4("000000000000", "00000000", "000000000000", "00000000")),
                         
        FASTREQ(1, 2, "hit macfwd, hit ipv4_lpm, hit nexthops", 
                         hETH(LPM_ETH2, ETH1A, cIPV4), 
                         hIP4("00", cIP4_0, "32000a02"))
        
        // {FAKE_PKT, 0,            0, IPV4("DDDDDDDD0000", "12345678", ETH01, .......TODO............),
        // {FAKE_PKT, 0,            0, ETH("DDDDDDDD0000", ETH01,          "000000000000000000000150", "12345678", "96000102"),
        //   NO_CTL_REPLY, 1, ETH("AABBBBAA0001", "EEDDDDAA0001", "0000000000000000FF000150", "12345678", "96000102")},
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test, "v1model" },
    TEST_SUITE_END,
};
