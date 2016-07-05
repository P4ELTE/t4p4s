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
#include <stdio.h>
#include <stdlib.h>
#include "vector.h"
#include <rte_malloc.h>

static void vector_init_elements(vector_t* vector, int from, int to) {
    int i;
    for(i = from; i < to; i++) {
        vector->data[i] = rte_malloc_socket("counter_array_element", vector->data_size, 0, vector->socketid);
        vector->value_init(vector->data[i]);
    }
}

void vector_init(vector_t *vector, int size, int capacity, int data_size, void (*value_init)(void *), int socketid) {
    vector->size = size;
    vector->socketid = socketid;
    vector->data_size = data_size;
    vector->capacity = capacity;
    vector->value_init = value_init;
    vector->data = rte_malloc_socket("counter_array", sizeof(void*) * vector->capacity, 0, socketid);
    vector_init_elements(vector, 0, size);
}

void vector_append(vector_t *vector, void* value) {
    vector_double_capacity_if_full(vector);
    vector->data[vector->size++] = value;
}

void* vector_get(vector_t *vector, int index) {
    if (index >= vector->size || index < 0) {
        printf("Index %d out of bounds for vector of size %d\n", index, vector->size);
        return NULL;
    }
    return vector->data[index];
}

void vector_set(vector_t *vector, int index, void* value) {
    while (index >= vector->size) {
        vector_append(vector, NULL);
    }
    vector->data[index] = value;
}

void vector_double_capacity_if_full(vector_t *vector) {
    if (vector->size >= vector->capacity) {
        int i = vector->capacity;
        vector->capacity *= 2;
        vector->data = rte_realloc(vector->data, vector->data_size * vector->capacity, 0);
        vector_init_elements(vector, i, vector->capacity);
    }
}

void vector_free(vector_t *vector) {
    free(vector->data);
}
