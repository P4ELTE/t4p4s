// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = 
	SINGLE_LCORE(
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
                         
        //FASTREQ(1, 22, "hit smac, hit dmac", hETH4(ETH02, ETH1A)),
        //FASTREQ(1, 33, "hit smac, hit dmac", hETH4(ETH03, ETH1A)),
        //FASTREQ(1, 44, "hit smac, hit dmac", hETH4(ETH04, ETH1A)),

        //FASTREQ(1, BCAST, "miss smac, miss dmac", hETH4(x6("01"), x6("10")))
);

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test, "v1model" },
    TEST_SUITE_END,
};
