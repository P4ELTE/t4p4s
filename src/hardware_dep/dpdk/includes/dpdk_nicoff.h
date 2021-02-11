// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#pragma once

#define T4P4S_NIC_VARIANT off

#include "test.h"
#include <stdbool.h>

struct lcore_data {
    struct lcore_conf*  conf;

    bool                is_valid;

    unsigned            idx;
    unsigned            pkt_idx;

    struct rte_mempool* mempool;
};
