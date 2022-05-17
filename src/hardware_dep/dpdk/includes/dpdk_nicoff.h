// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#pragma once

#define T4P4S_NIC_VARIANT off

#include "test_testsuite.h"
#include <stdbool.h>

struct lcore_data {
    struct lcore_conf*  conf;

    bool                is_valid;

    int                 idx;
    int                 verify_idx;
    int                 pkt_idx;
    int                 iter_idx;

    struct rte_mempool* mempool;
};


#define T4EXIT(code)    T4P4S_EXIT_CODE_ ## code


typedef enum {
    T4EXIT(OK) = 0,

    // note: these are external
    T4EXIT(COMPILE_MESON) = 1,
    T4EXIT(COMPILE_C) = 2,
    T4EXIT(COMPILE_P4) = 3,

    T4EXIT(LOOP) = 4,
    T4EXIT(DROP_SEND) = 5,
    T4EXIT(WRONG_OUTPUT) = 6,
    T4EXIT(CFLOW_REQ) = 7,
    T4EXIT(UNSET_EGRESS_PORT) = 11,
} T4EXIT(_t);
