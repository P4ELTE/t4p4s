// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

#define P4CALC_ETYPE "1234"
#define P4CALC_P     "50"   // 'P'
#define P4CALC_4     "34"   // '4'
#define P4CALC_VER   "01"   // v0.1
#define P4CALC_PLUS  "2b"   // '+'
#define P4CALC_MINUS "2d"   // '-'
#define P4CALC_AND   "26"   // '&'
#define P4CALC_OR    "7c"   // '|'
#define P4CALC_XOR   "5e"   // '^'

fake_cmd_t t4p4s_testcase_common[][RTE_MAX_LCORE] = SINGLE_LCORE(
    FAST(1, DROP, hETH4(ETH04, ETH1A)),
    
    FAST(1, DROP, hETH(ETH04, ETH1A, P4CALC_ETYPE), P4CALC_P, P4CALC_4, P4CALC_VER, "00", "00000002", "00000007", "00000000"),
    
    FAST(10, 10, IN(hETH(ETH04, ETH1A, P4CALC_ETYPE))
               , OUT(hETH(ETH1A, ETH04, P4CALC_ETYPE))
               , P4CALC_P, P4CALC_4, P4CALC_VER, P4CALC_PLUS, "00000002", "00000007", INOUT("00000000", "00000009")),
               
    FAST(10, 10, IN(hETH(ETH04, ETH1A, P4CALC_ETYPE))
               , OUT(hETH(ETH1A, ETH04, P4CALC_ETYPE))
               , P4CALC_P, P4CALC_4, P4CALC_VER, P4CALC_MINUS, "00000007", "00000002", INOUT("00000000", "00000005")),
               
    FAST(10, 10, IN(hETH(ETH04, ETH1A, P4CALC_ETYPE))
               , OUT(hETH(ETH1A, ETH04, P4CALC_ETYPE))
               , P4CALC_P, P4CALC_4, P4CALC_VER, P4CALC_AND, "00000001", "00000001", INOUT("00000000", "00000001")),
               
    FAST(10, 10, IN(hETH(ETH04, ETH1A, P4CALC_ETYPE))
               , OUT(hETH(ETH1A, ETH04, P4CALC_ETYPE))
               , P4CALC_P, P4CALC_4, P4CALC_VER, P4CALC_OR, "00000001", "00000010", INOUT("00000000", "00000011")),
               
    FAST(10, 10, IN(hETH(ETH04, ETH1A, P4CALC_ETYPE))
               , OUT(hETH(ETH1A, ETH04, P4CALC_ETYPE))
               , P4CALC_P, P4CALC_4, P4CALC_VER, P4CALC_XOR, "00000011", "00000010", INOUT("00000000", "00000001"))
    );


testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test", &t4p4s_testcase_common, "v1model" },
    { "psa",  &t4p4s_testcase_common, "psa" },
    TEST_SUITE_END,
};
