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

#include "hash_table.h"

#include "compiler.h"
#include "lib.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define HASH_MSB ((hash_t)(1ull << (8 * sizeof(hash_t) - 1)))
#define EMPTY_SLOT 0

struct hash_table
{
    struct hash_table_parameters parameters;
    int32_t bucket_count;
    int32_t bucket_mask;
    hash_t* hash_buckets;
    uint8_t* key_buckets;
};

struct lookup_data
{
    hash_t* hash_bucket;
    uint8_t* key_bucket;
    hash_t key_hash;
    int32_t bucket_index;
};

static always_inline hash_t get_hash(const struct hash_table* const table, const void* const key)
{
    return table->parameters.hash_function(key, table->parameters.key_size, table->parameters.hash_init_value);
}

static always_inline hash_t* get_hash_bucket(const struct hash_table* const table, const int32_t index)
{
    return &table->hash_buckets[index * table->parameters.bucket_entries];
}

static always_inline uint8_t* get_key_bucket(const struct hash_table* const table, const int32_t index)
{
    return &table->key_buckets[index * table->parameters.bucket_entries * table->parameters.key_size];
}

static always_inline uint8_t* get_key(const struct hash_table* const table, uint8_t* const bucket, const int32_t index)
{
    return &bucket[index * table->parameters.key_size];
}

static always_inline int keycmp(const struct hash_table* const table, const void* const a, const void* const b)
{
    return memcmp(a, b, table->parameters.key_size);
}

static always_inline int lookup_key(const struct hash_table* const table, const void* const key, struct lookup_data* const data)
{
    int32_t i;

    data->key_hash = get_hash(table, key) | HASH_MSB;
    data->bucket_index = data->key_hash & table->bucket_mask;
    data->hash_bucket = get_hash_bucket(table, data->bucket_index);
    data->key_bucket = get_key_bucket(table, data->bucket_index);

    for (i = 0; i < table->parameters.bucket_entries; ++i)
    {
        if (data->hash_bucket[i] == data->key_hash &&
            keycmp(table, key, get_key(table, data->key_bucket, i)) == 0)
        {
            return data->bucket_index * table->parameters.bucket_entries + i;
        }
    }

    return -1;
}

struct hash_table* hash_table_create(const struct hash_table_parameters* parameters)
{
    struct hash_table* table;
    int32_t bucket_count;

    if (parameters == NULL ||
        !IS_POWER_OF_TWO(parameters->entries) ||
        !IS_POWER_OF_TWO(parameters->bucket_entries) ||
        parameters->entries < 0 ||
        parameters->bucket_entries < 0 ||
        parameters->entries < parameters->bucket_entries)
    {
        errno = EINVAL;
        return NULL;
    }

    bucket_count = parameters->entries / parameters->bucket_entries;

    table = malloc(sizeof(*table));
    table->bucket_count = bucket_count;
    table->bucket_mask = bucket_count - 1;
    table->hash_buckets = calloc(parameters->entries, sizeof(hash_t));
    table->key_buckets = malloc(parameters->entries * parameters->key_size);

    memcpy(&table->parameters, parameters, sizeof(*parameters));

    return table;
}

void hash_table_free(struct hash_table* table)
{
    free(table->hash_buckets);
    free(table->key_buckets);
    free(table);
}

int32_t hash_table_add(struct hash_table* table, const void* key)
{
    struct lookup_data data;
    int32_t i;

    if (lookup_key(table, key, &data) >= 0)
    {
        errno = EEXIST;
        return -1;
    }
    
    for (i = 0; i < table->parameters.bucket_entries; ++i)
    {
        if (data.hash_bucket[i] == EMPTY_SLOT)
            goto add_key;
    }

    errno = ENOSPC;
    return -1;

add_key:
    data.hash_bucket[i] = data.key_hash;
    memcpy(get_key(table, data.key_bucket, i), key, table->parameters.key_size);

    return data.bucket_index * table->parameters.bucket_entries + i;
}

int32_t hash_table_remove(struct hash_table* table, const void* key)
{
    struct lookup_data data;
    int32_t i;

    i = lookup_key(table, key, &data);
    if (i < 0)
    {
        errno = ENOENT;
        return -1;
    }

    data.hash_bucket[i] = EMPTY_SLOT;
    
    return i;
}

int32_t hash_table_lookup(const struct hash_table* table, const void* key)
{
    struct lookup_data data;
    int32_t i;
    
    i = lookup_key(table, key, &data);
    if (i < 0)
    {
        errno = ENOENT;
        return -1;
    }

    return i;
}
