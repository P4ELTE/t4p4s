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
#ifndef TERNARY_NAIVE_H
#define TERNARY_NAIVE_H

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t* mask;
    uint8_t* key;
    uint8_t* value;
} ternary_entry;

typedef struct {
    void**  entries;
    uint8_t keylen;
    uint8_t size;
} ternary_table;

ternary_table* naive_ternary_create (uint8_t keylen, uint8_t max_size);
void           naive_ternary_destroy(ternary_table* t);
void           naive_ternary_add    (ternary_table* t, uint8_t* key, uint8_t* mask, uint8_t* value);
uint8_t*       naive_ternary_lookup (ternary_table* t, uint8_t* key);

#endif
