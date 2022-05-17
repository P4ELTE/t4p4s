// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include <stdint.h>
#include "common.h"

// ------------------------------------------------------
// Fake packet data

typedef enum {
    FAKE_PKT,
    // set default action
    FAKE_SETDEF,
    FAKE_END,
} fake_cmd_e;

#define MAX_SECTION_COUNT 128
#define MAX_REQ_LEN       4096

typedef struct {
    fake_cmd_e  action;
    void*       ptr;
    uint32_t    in_port;
    const char* in[MAX_SECTION_COUNT];

    int         sleep_millis;

    uint32_t    out_port;
    const char* out[MAX_SECTION_COUNT];

    // the [1] indicates that this part is either present or absent
    const char* requirements[1];
} fake_cmd_t;

// ------------------------------------------------------
// Test suite

typedef struct {
    const char* name;
    fake_cmd_t (*const steps)[][RTE_MAX_LCORE];
    const char* model;
} testcase_t;

#define TEST_SUITE_END { "", 0 }

#define MAX_TESTCASES 128

#define PAUSE_BETWEEN_TESTCASES_MILLIS 500

// ------------------------------------------------------
// Timeouts

#define INIT_WAIT_CONTROLPLANE_SHORT_MILLIS 500
#define INIT_WAIT_CONTROLPLANE_LONG_MILLIS  2000
#define WAIT_OTHER_CORE_PROCESSES_PACKAGES_MILLIS  500
