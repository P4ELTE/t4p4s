// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include <stdint.h>
#include "common.h"

// ------------------------------------------------------
// Fake packet data

enum fake_cmd_e {
    FAKE_PKT,
    // set default action
    FAKE_SETDEF,
    FAKE_END,
};

#define MAX_SECTION_COUNT 128

typedef struct {
    enum fake_cmd_e 			action;
    void*           			ptr;
    uint32_t        			in_port;
    const char*     			in[MAX_SECTION_COUNT];

    int             			sleep_millis;

    uint32_t        			out_port;
    const char*     			out[MAX_SECTION_COUNT];
    
    t4p4s_controlflow_name_t  	require[MAX_SECTION_COUNT];
    t4p4s_controlflow_name_t  	forbid[MAX_SECTION_COUNT];  
} fake_cmd_t;

// ------------------------------------------------------
// Test suite

typedef struct {
    const char* name;
    fake_cmd_t (*steps)[][RTE_MAX_LCORE];
} testcase_t;

#define TEST_SUITE_END { "", 0 }

#define MAX_TESTCASES 128

#define PAUSE_BETWEEN_TESTCASES_MILLIS 500

// ------------------------------------------------------
// Timeouts

#define INIT_WAIT_CONTROLPLANE_SHORT_MILLIS 500
#define INIT_WAIT_CONTROLPLANE_LONG_MILLIS  2000
#define WAIT_OTHER_CORE_PROCESSES_PACKAGES_MILLIS  500

// ------------------------------------------------------

#define T4P4S_BROADCAST_PORT 100
