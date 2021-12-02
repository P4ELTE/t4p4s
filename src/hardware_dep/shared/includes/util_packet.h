// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include "util_packet_bitfield.h"

#ifdef T4P4S_DEBUG
    #define KEYTXTPARAM        , const char* key_txt
    #define KEYTXTPARAMS       ,       char* key_txt, int* key_txt_idx
    #define KEYTXTPARAM_IN     , key_txt
    #define KEYTXTPARAMS_IN    KEYTXTPARAM_IN, &key_txt_idx
#else
    #define KEYTXTPARAM
    #define KEYTXTPARAMS
    #define KEYTXTPARAM_IN
    #define KEYTXTPARAMS_IN
#endif


#define MAX_BUF_PART_COUNT 16

typedef struct {
    int      size;
    uint8_t* buffer;

    int part_count;
    int part_bit_offsets[MAX_BUF_PART_COUNT];
    int part_bit_sizes[MAX_BUF_PART_COUNT];

    const char*const name;
    const char*const part_names[MAX_BUF_PART_COUNT];
} uint8_buffer_t;
