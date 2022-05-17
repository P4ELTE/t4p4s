// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

#define P4CALC_ETYPE "1234"
#define P4CALC_ID    "5034"  "0" "1"   // 'P4', version 0.1

#define P4CALC_PLUS  "2b"   // '+'
#define P4CALC_MINUS "2d"   // '-'
#define P4CALC_AND   "26"   // '&'
#define P4CALC_OR    "7c"   // '|'
#define P4CALC_XOR   "5e"   // '^'

#define P4CALC_PRE   hETH(INOUT(ETH04, ETH1A), INOUT(ETH1A, ETH04), P4CALC_ETYPE), P4CALC_ID
    
#define OPS(op1, op2) "000000" op1, "000000" op2
#define FROM0(out1B)  INOUT("00000000", "000000" out1B)
    
fake_cmd_t t4p4s_testcase_common[][RTE_MAX_LCORE] = SINGLE_LCORE(
    FAST(ANY, SAME, P4CALC_PRE, P4CALC_PLUS,  OPS("02", "07"), FROM0("09")),
    FAST(ANY, SAME, P4CALC_PRE, P4CALC_MINUS, OPS("07", "02"), FROM0("05")),
    FAST(ANY, SAME, P4CALC_PRE, P4CALC_AND,   OPS("01", "01"), FROM0("01")),
    FAST(ANY, SAME, P4CALC_PRE, P4CALC_OR,    OPS("01", "10"), FROM0("11")),
    FAST(ANY, SAME, P4CALC_PRE, P4CALC_XOR,   OPS("11", "10"), FROM0("01"))
    );


testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test", &t4p4s_testcase_common, "v1model" },
    { "psa",  &t4p4s_testcase_common, "psa" },
    TEST_SUITE_END,
};
