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

#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stddef.h>
#include <stdint.h>

typedef uint32_t hash_t;
typedef hash_t (*hash_func_t)(const void* data, size_t size, hash_t init_value);

struct hash_table_parameters
{
    hash_func_t hash_function;
    hash_t hash_init_value;
    int32_t entries;
    int32_t bucket_entries;
    size_t key_size;
};

struct hash_table* hash_table_create(const struct hash_table_parameters* parameters);
void hash_table_free(struct hash_table* table);

int32_t hash_table_add(struct hash_table* table, const void* key);
int32_t hash_table_remove(struct hash_table* table, const void* key);
int32_t hash_table_lookup(const struct hash_table* table, const void* key);

#endif
