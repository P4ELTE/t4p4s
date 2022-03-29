// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = SINGLE_LCORE(
    FAST(0, 0,
            IN(hETH4("DDDDDDDDDDDD", "EEEEEEEEEEEE")), // dddd dddd dddd eeee eeee eeee 0800 #len: 14
            IN(hMISC(IPv4, "4500001600010000400074e2" "01010101" "02020202")), // 4500 0016 0001 0000 4000 74e2 0101 0101 0202 0202 #len: 20
            IN(PAYLOAD("abcd")),
            OUT(hETH4("DDDDDDDDDDDD", "EEEEEEEEEEEE")),
            OUT(hMISC(IPv4, "450000580001000040326c66" "03030303" "04040404")),
            OUT(hMISC(ESPHead, "0000022200000001")),
            OUT(hMISC(IV, "00000000000000000000000000000000")),
            OUT(hMISC(IPsecEncryptedPayload,"95ad5d83be4a49a5bf9e27bf63533ca7b1a59daaa8b167f3f2fcb7473d74ee50"))
            OUT(hMISC(ESPAuth,"38cd6ad9f80b6c208c8d3682"))
        )
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test", &t4p4s_testcase_test, "v1model" },
    TEST_SUITE_END,
};
