// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include "aliases.h"

enum lookup_t {
    LOOKUP_none,

    LOOKUP_exact,
    LOOKUP_lpm,
    LOOKUP_ternary,
};

struct type_field_list {
    uint8_t fields_quantity;
    uint8_t** field_offsets;
    uint8_t* field_widths;
};

typedef struct lookup_table_entry_info_s {
    int entry_count;

    uint8_t key_size;

    // entry size == val_size + validity_size + state_size
    uint8_t entry_size;
    uint8_t action_size;
    uint8_t validity_size;
    uint8_t state_size;
} lookup_table_entry_info_t;

typedef struct lookup_table_s {
    char* name;
    char* canonical_name;

    unsigned id;
    uint8_t type;
    bool is_hidden;

    int min_size;
    int max_size;

    void* default_val;
    void* table;

    int socketid;
    int instance;

    lookup_table_entry_info_t entry;
#ifdef T4P4S_DEBUG
    int init_entry_count;
#endif
} lookup_table_t;
