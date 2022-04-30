// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_common[][RTE_MAX_LCORE] = SINGLE_LCORE(
    FAST(0, 0, "00", INOUT(hMISC(unencrypted, "68656c6c6f776f726c64617364313200"), hMISC(encrypted, "a5c06e42980a6dbbea2fd2c45a94bc77"))),
    FAST(0, 0, "01", INOUT(hMISC(unencrypted, "1168656c6c6f776f726c64617364313200"), hMISC(encrypted, "11a5c06e42980a6dbbea2fd2c45a94bc77"))),
    FAST(0, 0, "02", INOUT(hMISC(unencrypted, "112268656c6c6f776f726c64617364313200"), hMISC(encrypted, "1122a5c06e42980a6dbbea2fd2c45a94bc77"))),
    FAST(0, 0, "03", INOUT(hMISC(unencrypted, "11223368656c6c6f776f726c64617364313200"), hMISC(encrypted, "112233a5c06e42980a6dbbea2fd2c45a94bc77"))),
    FAST(0, 0, "04", INOUT(hMISC(unencrypted, "1122334468656c6c6f776f726c64617364313200"), hMISC(encrypted, "11223344a5c06e42980a6dbbea2fd2c45a94bc77"))),
    FAST(0, 0, "0b", INOUT(hMISC(unencrypted, "112233445566778899aabb68656c6c6f776f726c64617364313200"), hMISC(encrypted, "112233445566778899aabba5c06e42980a6dbbea2fd2c45a94bc77"))),

    FAST(0, 0, "00", INOUT(hMISC(unencrypted, "68656c6c6f0000000000000000000000"), hMISC(encrypted, "10fb1c3fed5a1d4aa8d60b955b09ff02"))),
    FAST(0, 0, "01", INOUT(hMISC(unencrypted, "68656c6c6f776f726c6461736431323300"), hMISC(encrypted, "68cad90cc0231405a0c0d880630b34facf"))),
    FAST(0, 0, "01", INOUT(hMISC(unencrypted, "68656c6c6f776f726c6461736400000000"), hMISC(encrypted, "685d8f891abb54cbfe8ef3a330f0853bd5"))),
    FAST(0, 0, "02", INOUT(hMISC(unencrypted, "2268656c6c6f776f726c6461736400000000"), hMISC(encrypted, "22685d8f891abb54cbfe8ef3a330f0853bd5"))),
    FAST(0, 0, "03", INOUT(hMISC(unencrypted, "332268656c6c6f776f726c6461736400000000"), hMISC(encrypted, "3322685d8f891abb54cbfe8ef3a330f0853bd5"))),
    FAST(0, 0, "05", INOUT(hMISC(unencrypted, "5544332268656c6c6f776f726c6461736400000000"), hMISC(encrypted, "55443322685d8f891abb54cbfe8ef3a330f0853bd5"))),

    FAST(0, 0, "01", INOUT(hMISC(unencrypted, "68656c6c6f776f726c64617364"), hMISC(encrypted, "685d8f891abb54cbfe8ef3a330f0853bd5"))),
    FAST(0, 0, "02", INOUT(hMISC(unencrypted, "2268656c6c6f776f726c64617364"), hMISC(encrypted, "22685d8f891abb54cbfe8ef3a330f0853bd5"))),
    FAST(0, 0, "03", INOUT(hMISC(unencrypted, "332268656c6c6f776f726c64617364"), hMISC(encrypted, "3322685d8f891abb54cbfe8ef3a330f0853bd5"))),
    FAST(0, 0, "05", INOUT(hMISC(unencrypted, "5544332268656c6c6f776f726c64617364"), hMISC(encrypted, "55443322685d8f891abb54cbfe8ef3a330f0853bd5")))
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "v1", &t4p4s_testcase_common, "v1model" },
    { "psa", &t4p4s_testcase_common, "psa" },
    TEST_SUITE_END,
};
