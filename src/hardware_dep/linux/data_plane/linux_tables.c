// Copyright 2016 Eotvos Lorand University, Budapest, Hungary
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "bitwise_trie.h"
#include "compiler.h"
#include "config.h"
#include "crc32.h"
#include "lib.h"
#include "linux_backend.h"
#include "hash_table.h"
#include "ternary_naive.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct extended_table_s {
    void*     table;
    uint8_t   size;
    uint8_t** content;
} extended_table_t;

#if CRC32_INSTRUCTION_SUPPORTED
static DEFINE_CRC32_SSE(1);
static DEFINE_CRC32_SSE(2);
static DEFINE_CRC32_SSE(3);
static DEFINE_CRC32_SSE(4);
static DEFINE_CRC32_SSE(5);
static DEFINE_CRC32_SSE(6);
static DEFINE_CRC32_SSE(7);
static DEFINE_CRC32_SSE(8);

static const hash_func_t crc32_sse_fixed_size[9] =
{
    NULL,
    CRC32_SSE_NAME(1),
    CRC32_SSE_NAME(2),
    CRC32_SSE_NAME(3),
    CRC32_SSE_NAME(4),
    CRC32_SSE_NAME(5),
    CRC32_SSE_NAME(6),
    CRC32_SSE_NAME(7),
    CRC32_SSE_NAME(8)
};
#endif

// ============================================================================
// HIGHER LEVEL TABLE MANAGEMENT

// ----------------------------------------------------------------------------
// CREATE

static void create_ext_table(lookup_table_t* t, void* table)
{
    extended_table_t* const ext = malloc(sizeof(extended_table_t));
    ext->table = table;
    ext->size = 0;
    ext->content = malloc(TABLE_CONTENT_MAX * sizeof(uint8_t*));

    t->table = ext;
}

void exact_create(lookup_table_t* t, unused int socketid)
{
    const struct hash_table_parameters params =
    {
#if CRC32_INSTRUCTION_SUPPORTED
        .hash_function = t->key_size <= 8 ? crc32_sse_fixed_size[t->key_size] : crc32_sse,
#else
        .hash_function = crc32,
#endif
        .hash_init_value = 0,
        .entries = HASH_TABLE_ENTRIES,
        .bucket_entries = HASH_TABLE_BUCKET_ENTRIES,
        .key_size = t->key_size
    };

    create_ext_table(t, hash_table_create(&params));
}

void lpm_create(lookup_table_t* t, unused int socketid)
{
    const struct bitwise_trie_parameters params =
    {
        .default_value = -1,
        .key_size = t->key_size
    };

    create_ext_table(t, bitwise_trie_create(&params));
}

void ternary_create(lookup_table_t* t, unused int socketid)
{
    t->table = naive_ternary_create(t->key_size, t->max_size);
}

// ----------------------------------------------------------------------------
// ADD

static uint8_t* add_index(uint8_t* value, size_t val_size, int index)
{
    uint8_t* const value_copy = malloc(val_size + sizeof(int));
    memcpy(value_copy, value, val_size);
    *(int*)(value_copy + val_size) = index;

    return value_copy;
}

void exact_add(lookup_table_t* t, uint8_t* key, uint8_t* value)
{
    if (t->key_size == 0)
        return;

    extended_table_t* const ext = t->table;
    const int32_t index = hash_table_add(ext->table, key);

    if (index < 0)
    {
        print_errno("hash_table_add");
        return;
    }
    
    ext->content[index & (TABLE_CONTENT_MAX - 1)] = add_index(value, t->val_size, t->counter++);
}

void lpm_add(lookup_table_t* t, uint8_t* key, uint8_t depth, uint8_t* value)
{
    if (t->key_size == 0)
        return;

    extended_table_t* const ext = t->table;

    if (bitwise_trie_add(ext->table, key, depth, ext->size) < 0)
    {
        print_errno("bitwise_trie_add");
        return;
    }

    ext->content[ext->size++] = add_index(value, t->val_size, t->counter++);
}

void ternary_add(lookup_table_t* t, uint8_t* key, uint8_t* mask, uint8_t* value)
{
    if (t->key_size == 0)
        return;

    naive_ternary_add(t->table, key, mask, add_index(value, t->val_size, t->counter++));
}

// ----------------------------------------------------------------------------
// SET DEFAULT VALUE

void table_setdefault(lookup_table_t* t, uint8_t* value)
{
    t->default_val = add_index(value, t->val_size, DEFAULT_ACTION_INDEX);
}

// ----------------------------------------------------------------------------
// LOOKUP

uint8_t* exact_lookup(lookup_table_t* t, uint8_t* key)
{
    if (t->key_size == 0)
        return t->default_val;

    const extended_table_t* const ext = t->table;
    const int32_t ret = hash_table_lookup(ext->table, key);

    return (ret < 0) ? t->default_val : ext->content[ret & (TABLE_CONTENT_MAX - 1)];
}

uint8_t* lpm_lookup(lookup_table_t* t, uint8_t* key)
{
    if (t->key_size == 0)
        return t->default_val;

    const extended_table_t* const ext = t->table;
    const int ret = bitwise_trie_lookup(ext->table, key);

    return (ret < 0) ? t->default_val : ext->content[ret];
}

uint8_t* ternary_lookup(lookup_table_t* t, uint8_t* key)
{
    if (t->key_size == 0)
        return t->default_val;

    uint8_t* const ret = naive_ternary_lookup(t->table, key);
    
    return ret == NULL ? t->default_val : ret;
}
