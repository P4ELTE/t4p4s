// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

// This part of the payload is encrypted then decrypted, resulting in identical content.
#define encrypt_then_decrypt(in)   INOUT(in, in)

fake_cmd_t t4p4s_testcase_common[][RTE_MAX_LCORE] = SINGLE_LCORE(
    FAST(0, SAME, "00", PAYLOAD(                         encrypt_then_decrypt("68656c6c6f776f726c64617364313200"))),
    FAST(0, SAME, "01", PAYLOAD("11"                     encrypt_then_decrypt("68656c6c6f776f726c64617364313200"))),
    FAST(0, SAME, "02", PAYLOAD("1122"                   encrypt_then_decrypt("68656c6c6f776f726c64617364313200"))),
    FAST(0, SAME, "03", PAYLOAD("112233"                 encrypt_then_decrypt("68656c6c6f776f726c64617364313200"))),
    FAST(0, SAME, "04", PAYLOAD("11223344"               encrypt_then_decrypt("68656c6c6f776f726c64617364313200"))),
    FAST(0, SAME, "0b", PAYLOAD("112233445566778899aabb" encrypt_then_decrypt("68656c6c6f776f726c64617364313200"))),

    FAST(0, SAME, "00", PAYLOAD(             encrypt_then_decrypt("68656c6c6f0000000000000000000000"))),
    FAST(0, SAME, "01", PAYLOAD("68"         encrypt_then_decrypt("656c6c6f776f726c6461736431323300"))),
    FAST(0, SAME, "01", PAYLOAD("68"         encrypt_then_decrypt("656c6c6f776f726c6461736400000000"))),
    FAST(0, SAME, "02", PAYLOAD("2268"       encrypt_then_decrypt("656c6c6f776f726c6461736400000000"))),
    FAST(0, SAME, "03", PAYLOAD("332268"     encrypt_then_decrypt("656c6c6f776f726c6461736400000000"))),
    FAST(0, SAME, "05", PAYLOAD("5544332268" encrypt_then_decrypt("656c6c6f776f726c6461736400000000")))
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "v1", &t4p4s_testcase_common, "v1model" },
    { "psa", &t4p4s_testcase_common, "psa" },
    TEST_SUITE_END,
};
