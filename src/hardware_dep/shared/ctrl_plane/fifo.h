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
#ifndef __FIFO_H__
#define __FIFO_H__

#include <pthread.h>

#define P4_BG_QUEUE_SIZE 1024

typedef struct fifo_st {
        void* queue[P4_BG_QUEUE_SIZE];
        int head;
        int size;
        int tail;
        pthread_mutex_t lock;
        pthread_cond_t not_empty;     /*non empty and empty condidtion variables*/
        pthread_cond_t empty;
} fifo_t;

fifo_t* fifo_init( fifo_t* queue );
void fifo_destroy( fifo_t* queue );
fifo_t* fifo_add_msg( fifo_t* queue, void* element);
void* fifo_remove_msg( fifo_t* queue );
int fifo_size( fifo_t* queue );
void fifo_wait( fifo_t* queue );
int fifo_isfull( fifo_t* queue );
#endif
