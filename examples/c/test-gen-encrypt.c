// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

#define ENCRYPT              "01"
#define DECRYPT              "02"
#define ENCRYPT_THEN_DECRYPT "03"

#define cENCRYPTED1 "40d5a3020100000040d5636b000000"
#define cENCRYPTED2 "40d5a3020100000040d5636b000000"
#define cENCRYPTED3 "40d5a3020100000040d5636b000000"
#define cENCRYPTED4 "40d5a3020100000040d5636b000000"

fake_cmd_t t4p4s_testcase_common[][RTE_MAX_LCORE] = SINGLE_LCORE(
    FAST(0, 0, IN(hMISC(op, ENCRYPT)), IN("01"), INOUT(hETH4(ETH01, ETH1A), PAYLOAD(cENCRYPTED1))),
    FAST(0, 0, IN(hMISC(op, ENCRYPT)), IN("01"), INOUT(hETH4(ETH02, ETH1A), PAYLOAD(cENCRYPTED2))),
    FAST(0, 0, IN(hMISC(op, ENCRYPT)), IN("01"), INOUT(hETH4(ETH03, ETH1A), PAYLOAD(cENCRYPTED3))),
    FAST(0, 0, IN(hMISC(op, ENCRYPT)), IN("01"), INOUT(hETH4(ETH04, ETH1A), PAYLOAD(cENCRYPTED4))),

    FAST(0, 0, IN(hMISC(op, DECRYPT)), INOUT(PAYLOAD(cENCRYPTED1), hETH4(ETH01, ETH1A))),
    FAST(0, 0, IN(hMISC(op, DECRYPT)), INOUT(PAYLOAD(cENCRYPTED2), hETH4(ETH02, ETH1A))),
    FAST(0, 0, IN(hMISC(op, DECRYPT)), INOUT(PAYLOAD(cENCRYPTED3), hETH4(ETH03, ETH1A))),
    FAST(0, 0, IN(hMISC(op, DECRYPT)), INOUT(PAYLOAD(cENCRYPTED4), hETH4(ETH04, ETH1A))),

    FAST(0, 0, IN(hMISC(op, ENCRYPT_THEN_DECRYPT)), IN("01"), hETH4(ETH01, ETH1A)),
    FAST(0, 0, IN(hMISC(op, ENCRYPT_THEN_DECRYPT)), IN("01"), hETH4(ETH02, ETH1A)),
    FAST(0, 0, IN(hMISC(op, ENCRYPT_THEN_DECRYPT)), IN("01"), hETH4(ETH03, ETH1A)),
    FAST(0, 0, IN(hMISC(op, ENCRYPT_THEN_DECRYPT)), IN("01"), hETH4(ETH04, ETH1A))
    );


testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test", &t4p4s_testcase_common, "v1model" },
    { "psa",  &t4p4s_testcase_common, "psa" },
    TEST_SUITE_END,
};
