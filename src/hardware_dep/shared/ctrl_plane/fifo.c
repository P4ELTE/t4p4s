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
#include "fifo.h"
#include <stdio.h>

fifo_t* fifo_init( fifo_t* queue)
{
        queue->head = 0;
        queue->size = 0;
        queue->tail = -1;

        /* initialize mutex and condition variables. */
        if(pthread_mutex_init(&(queue->lock),NULL)) {
                fprintf(stderr, "Mutex initiation error!\n");
                return 0;
        }
        if(pthread_cond_init(&(queue->empty),NULL)) {
                fprintf(stderr, "CV initiation error!\n");
                return 0;
        }
        if(pthread_cond_init(&(queue->not_empty),NULL)) {
                fprintf(stderr, "CV initiation error!\n");
                return 0;
        }
        return queue;
}

void fifo_destroy( fifo_t* queue )
{
        pthread_mutex_destroy(&(queue->lock));
        pthread_cond_destroy(&(queue->empty));
        pthread_cond_destroy(&(queue->not_empty));
}

fifo_t* fifo_add_msg( fifo_t* queue, void* element)
{
        pthread_mutex_lock(&(queue->lock));
                if (queue->size < P4_BG_QUEUE_SIZE)
                {
                        queue->tail = (queue->tail+1) % P4_BG_QUEUE_SIZE;
                        ++(queue->size);
                        queue->queue[queue->tail] = element;
                        pthread_cond_signal(&(queue->not_empty));
                }
        pthread_mutex_unlock(&(queue->lock));

        return queue;
}

void* fifo_remove_msg( fifo_t* queue )
{
        void* result = 0;
        pthread_mutex_lock(&(queue->lock));
                if (queue->size > 0)
                {
                        result = queue->queue[queue->head];
                        queue->head = (queue->head+1) % P4_BG_QUEUE_SIZE;
                        --(queue->size);
                        if (queue->size==0)
                                pthread_cond_signal(&(queue->empty));
                }
        pthread_mutex_unlock(&(queue->lock));

        return result;
}

int fifo_size( fifo_t* queue )
{
        int size = 0;
        pthread_mutex_lock(&(queue->lock));
        size = queue->size;
        pthread_mutex_unlock(&(queue->lock));

        return size;
}

void fifo_wait( fifo_t* queue)
{
        pthread_mutex_lock(&(queue->lock));

        if (queue->size==0)
                pthread_cond_wait(&(queue->not_empty), &(queue->lock));

        pthread_mutex_unlock(&(queue->lock));
}
