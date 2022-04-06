// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = SINGLE_LCORE(
        WAIT_FOR_CTL,
        
        FAST(1, DROP, hETH(ETH04, ETH1A, cIPV4), 
                                        hIP4("00", cIP4_0, cIP4_0)),
                                              
        FAST(1, 15, hETH(LPM_ETH2, ETH1A, cIPV4), 
                    hIP4("00", cIP4_0, "0a006363"))
                                              
        
        //FAST(0, 0, ETH("DDDDDDDD0000", ETH01, "0000000000000000000029f0", "12345678", "0a006363"), 200, 15, ETH(L3_MAC1, L3_MAC2, "0000000000000000000029f0", "12345678", "0a006363")),
        
        //FAST(0, 0, ETH(ETH1A, ETH04, IPV4_FFFF), 200, 15, ETH(L3_MAC1, L3_MAC2, "00000000000000000000ffff0000000000000000"))
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test, "v1model" },
    TEST_SUITE_END,
};
