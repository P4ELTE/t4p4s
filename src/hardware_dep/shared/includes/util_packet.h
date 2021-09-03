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

typedef struct {
    int      buffer_size;
    uint8_t* buffer;
} uint8_buffer_t;
