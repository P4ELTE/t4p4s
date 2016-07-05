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
#ifndef VECTOR_H
#define VECTOR_H

typedef struct vector_s {
    int size;
    int capacity;
    int data_size;
    void (*value_init)(void *);
    void **data;
    int socketid;
} vector_t;

void vector_init(vector_t *vector, int size, int capacity, int data_size, void (*value_init)(void *), int socketid);
void vector_append(vector_t *vector, void* value);
void*  vector_get(vector_t *vector, int index);
void vector_set(vector_t *vector, int index, void* value);
void vector_double_capacity_if_full(vector_t *vector);
void vector_free(vector_t *vector);

#endif // VECTOR_H
