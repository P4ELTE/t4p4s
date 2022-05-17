// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

#define encrypt1    op(encrypt, "68656c6c6f776f726c64617364313200", "a5c06e42980a6dbbea2fd2c45a94bc77")
#define encrypt2    op(encrypt, "68656c6c6f0000000000000000000000", "10fb1c3fed5a1d4aa8d60b955b09ff02")
#define encrypt3    op(encrypt, "656c6c6f776f726c6461736431323300", "cad90cc0231405a0c0d880630b34facf")
#define encrypt4    op(encrypt, "656c6c6f776f726c6461736400000000", "5d8f891abb54cbfe8ef3a330f0853bd5")

fake_cmd_t t4p4s_testcase_common[][RTE_MAX_LCORE] = SINGLE_LCORE(
    FAST(0, SAME, "00", PAYLOAD(                          encrypt1)),
    FAST(0, SAME, "01", PAYLOAD("11",                     encrypt1)),
    FAST(0, SAME, "02", PAYLOAD("1122",                   encrypt1)),
    FAST(0, SAME, "03", PAYLOAD("112233",                 encrypt1)),
    FAST(0, SAME, "04", PAYLOAD("11223344",               encrypt1)),
    FAST(0, SAME, "0b", PAYLOAD("112233445566778899aabb", encrypt1)),

    FAST(0, SAME, "00", PAYLOAD(              encrypt2)),
    FAST(0, SAME, "01", PAYLOAD("68",         encrypt3)),
    FAST(0, SAME, "01", PAYLOAD("68",         encrypt4)),
    FAST(0, SAME, "02", PAYLOAD("2268",       encrypt4)),
    FAST(0, SAME, "03", PAYLOAD("332268",     encrypt4)),
    FAST(0, SAME, "05", PAYLOAD("5544332268", encrypt4))
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "v1", &t4p4s_testcase_common, "v1model" },
    { "psa", &t4p4s_testcase_common, "psa" },
    TEST_SUITE_END,
};
