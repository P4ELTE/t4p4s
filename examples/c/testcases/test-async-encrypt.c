// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_common[][RTE_MAX_LCORE] = SINGLE_LCORE(
    FAST(0, 0, "00" INOUT(hMISC(unencrypted, "68656c6c6f776f726c64617364313200"), hMISC(encrypted, "a5c06e42980a6dbbea2fd2c45a94bc77"))),
    FAST(0, 0, "01" "11" INOUT(hMISC(unencrypted, "68656c6c6f776f726c64617364313200"), hMISC(encrypted, "a5c06e42980a6dbbea2fd2c45a94bc77"))),
    FAST(0, 0, "02" "1122" INOUT(hMISC(unencrypted, "68656c6c6f776f726c64617364313200"), hMISC(encrypted, "a5c06e42980a6dbbea2fd2c45a94bc77"))),
    FAST(0, 0, "03" "112233" INOUT(hMISC(unencrypted, "68656c6c6f776f726c64617364313200"), hMISC(encrypted, "a5c06e42980a6dbbea2fd2c45a94bc77"))),
    FAST(0, 0, "04" "11223344" INOUT(hMISC(unencrypted, "68656c6c6f776f726c64617364313200"), hMISC(encrypted, "a5c06e42980a6dbbea2fd2c45a94bc77"))),
    FAST(0, 0, "0b" "112233445566778899aabb" INOUT(hMISC(unencrypted, "68656c6c6f776f726c64617364313200"), hMISC(encrypted, "a5c06e42980a6dbbea2fd2c45a94bc77"))),

    FAST(0, 0, "00" INOUT(hMISC(unencrypted, "68656c6c6f0000000000000000000000"), hMISC(encrypted, "10fb1c3fed5a1d4aa8d60b955b09ff02"))),
    FAST(0, 0, "01" "68" INOUT(hMISC(unencrypted, "656c6c6f776f726c6461736431323300"), hMISC(encrypted, "cad90cc0231405a0c0d880630b34facf"))),
    FAST(0, 0, "01" "68" INOUT(hMISC(unencrypted, "656c6c6f776f726c6461736400000000"), hMISC(encrypted, "5d8f891abb54cbfe8ef3a330f0853bd5"))),
    FAST(0, 0, "02" "2268" INOUT(hMISC(unencrypted, "656c6c6f776f726c6461736400000000"), hMISC(encrypted, "5d8f891abb54cbfe8ef3a330f0853bd5"))),
    FAST(0, 0, "03" "332268" INOUT(hMISC(unencrypted, "656c6c6f776f726c6461736400000000"), hMISC(encrypted, "5d8f891abb54cbfe8ef3a330f0853bd5"))),
    FAST(0, 0, "05" "5544332268" INOUT(hMISC(unencrypted, "656c6c6f776f726c6461736400000000"), hMISC(encrypted, "5d8f891abb54cbfe8ef3a330f0853bd5")))
    );

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "v1", &t4p4s_testcase_common, "v1model" },
    { "psa", &t4p4s_testcase_common, "psa" },
    TEST_SUITE_END,
};
