// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t* pointer;

    int      bitwidth;
    int      bytewidth;
    int      bitcount;
    int      bytecount;
    int      bitoffset;
    int      byteoffset;
    uint32_t mask;

    bool     fixed_width;
    bool     is_t4p4s_byte_order;

    bool     is_ok;
} bitfield_handle_t;
