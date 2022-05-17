// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

#define LOOKUP_CONST_LPM4_ENTRY "00"
#define ADD_ENTRY               "01"
#define LOOKUP_LPM4_ENTRY       "02"

#define RESULT_MISSED_LOOKUP    OUT("00")

fake_cmd_t t4p4s_testcase_common[][RTE_MAX_LCORE] = SINGLE_LCORE(
    FAST(ANY, SAME, IN(LOOKUP_CONST_LPM4_ENTRY), IN("20"), IN("00000000"), RESULT_MISSED_LOOKUP),
    FAST(ANY, SAME, IN(LOOKUP_CONST_LPM4_ENTRY), IN("20"), IN("DEADC0DE"), RESULT_MISSED_LOOKUP),

    // lookups expected to succeed
    FAST(ANY, SAME, IN(LOOKUP_CONST_LPM4_ENTRY), IN("20"), IN("00112233"), OUT("01")),
    FAST(ANY, SAME, IN(LOOKUP_CONST_LPM4_ENTRY), IN("20"), IN("44556677"), OUT("02")),
    FAST(ANY, SAME, IN(LOOKUP_CONST_LPM4_ENTRY), IN("20"), IN("32000a02"), OUT("03"))
    );


testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test", &t4p4s_testcase_common, "v1model" },
    { "psa",  &t4p4s_testcase_common, "psa" },
    TEST_SUITE_END,
};
