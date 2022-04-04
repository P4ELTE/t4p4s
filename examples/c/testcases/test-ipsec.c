// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = SINGLE_LCORE(
        FAST(0, 0,
             IN(hETH4("DDDDDDDDDDDD", "EEEEEEEEEEEE")), // dddd dddd dddd eeee eeee eeee 0800 #len: 14
             IN(hMISC(IPv4, "4500" "0016" "0001" "0000" "40" "00" "74e2" "01010101" "02020202")), // 4500 0016 0001 0000 4000 74e2 0101 0101 0202 0202 #len: 20
             IN(PAYLOAD("abcd")),
             OUT(hETH4("DDDDDDDDDDDD", "EEEEEEEEEEEE")),
             OUT(hMISC(IPv4, "4500" "0058" "0001" "0000" "40" "32" "6c66" "03030303" "04040404")),
             OUT(hMISC(ESPHead, "0000022200000001")),
             OUT(hMISC(IV, "00000000000000000000000000000000")),
             OUT(hMISC(IPsecEncryptedPayload,"95ad5d83be4a49a5bf9e27bf63533ca7b1a59daaa8b167f3f2fcb7473d74ee50"))
             OUT(hMISC(ESPAuth,"38cd6ad9f80b6c208c8d3682"))
        ),
        FAST(0, 0,
             IN(hETH4("DDDDDDDDDDDD", "EEEEEEEEEEEE")),
             IN(hMISC(IPv4, "4500" "0016" "0001" "0000" "40" "00" "4cb6" "0a0b0c0d" "0b0c0d0e")),
             IN(PAYLOAD("abcd")),
             OUT(hETH4("DDDDDDDDDDDD", "EEEEEEEEEEEE")),
             OUT(hMISC(IPv4, "4500" "0058" "0001" "0000" "40" "32" "6c66" "03030303" "04040404")),
             OUT(hMISC(ESPHead, "0000022200000001")),
             OUT(hMISC(IV, "00000000000000000000000000000000")),
             OUT(hMISC(IPsecEncryptedPayload,"1348439d6f966becf6c79b6e220828e7240c80d2a14c738130920645a4ece511"))
                OUT(hMISC(ESPAuth,"200a553288674d594fb7388d"))
        ),
        FAST(0, 0,
             IN(hETH4("DDDDDDDDDDDD", "EEEEEEEEEEEE")),
             IN(hMISC(IPv4, "4500" "001c" "0001" "0000" "40" "00" "4cb0" "0a0b0c0d" "0b0c0d0e")),
             IN(PAYLOAD("123456789abcdef0")),
             OUT(hETH4("DDDDDDDDDDDD", "EEEEEEEEEEEE")),
             OUT(hMISC(IPv4, "4500" "0058" "0001" "0000" "40" "32" "6c66" "03030303" "04040404")),
             OUT(hMISC(ESPHead, "0000022200000001")),
             OUT(hMISC(IV, "00000000000000000000000000000000")),
             OUT(hMISC(IPsecEncryptedPayload,"bc58ff280ee6905c40883d054fb4743f3b2b3bca40c6a96fa172157162a44b89"))
                OUT(hMISC(ESPAuth,"a15828126eb9c93ab654cb95"))
        ),
        FAST(0, 0,
                IN(hETH4("DDDDDDDDDDDD", "EEEEEEEEEEEE")),
                IN(hMISC(IPv4, "4500" "002c" "0001" "0000" "40" "00" "4ca0" "0a0b0c0d" "0b0c0d0e")),
                IN(PAYLOAD("123456789abcdef0123456789abcdef0123456789abcdef0")),
                OUT(hETH4("DDDDDDDDDDDD", "EEEEEEEEEEEE")),
                OUT(hMISC(IPv4, "4500" "0068" "0001" "0000" "40" "32" "6c56" "03030303" "04040404")),
                OUT(hMISC(ESPHead, "0000022200000001")),
                OUT(hMISC(IV, "00000000000000000000000000000000")),
                OUT(hMISC(IPsecEncryptedPayload,"e3c5c76371cb1b096a232405e620238b8c48c54a492974c54a3e54332a4a402cd17f0bbb9e1ed11ad6f5477d3e86e184"))
                OUT(hMISC(ESPAuth,"1c4296baa6c9f4d115c775a0"))
        )

);

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test", &t4p4s_testcase_test, "v1model" },
    TEST_SUITE_END,
};
