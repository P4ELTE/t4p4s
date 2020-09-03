// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_controller_dictionary.h"

#include <string.h>
#include <stdio.h>

static int translation_table_idx = 0;
static char translation_table[100][2][256] = { {"", ""} };

void add_translation(const char* key, const char* value) {
    strcpy(translation_table[translation_table_idx][0], key);
    strcpy(translation_table[translation_table_idx][1], value);
    ++translation_table_idx;
}

const char* translate(const char* key) {
    for (int i = translation_table_idx - 1; i >= 0; --i)
    {
        if (strcmp(key, translation_table[i][0]) == 0) {
            return translation_table[i][1];
        }
    }

    return key;
}

void print_translations() {
    for (int i = 0; i < translation_table_idx; ++i)
    {
        const char* key = translation_table[i][0];
        const char* value = translation_table[i][1];
        const char* output = strcmp(key, value) == 0 ? "(same)" : value;
        printf("    %20s -> %s\n", key, output);
    }
}
