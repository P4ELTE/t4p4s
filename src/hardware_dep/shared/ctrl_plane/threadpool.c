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
/**
 * threadpool.c
 *
 * This file will contain your implementation of a threadpool.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include "threadpool.h"


extern int pthread_kill(pthread_t thread, int sig);

typedef struct work_st{
	void (*routine) (void*);
	void * arg;
	struct work_st* next;
} work_t;

typedef struct _threadpool_st {
   
	int num_threads;	
	int qsize;			
	pthread_t *threads;	
	work_t* qhead;		
	work_t* qtail;		
	pthread_mutex_t qlock;		
	pthread_cond_t q_not_empty;	
	pthread_cond_t q_empty;
	int shutdown;
	int dont_accept;
} _threadpool;

/* This function is the work function of the thread */
void* do_work(threadpool p) {
	_threadpool * pool = (_threadpool *) p;
	work_t* cur;	

	while(1) {
		pool->qsize = pool->qsize;
		pthread_mutex_lock(&(pool->qlock));  


		while( pool->qsize == 0) {	
			if(pool->shutdown) {
				pthread_mutex_unlock(&(pool->qlock));
				pthread_exit(NULL);
			}
			
			pthread_mutex_unlock(&(pool->qlock));  
			pthread_cond_wait(&(pool->q_not_empty),&(pool->qlock));

			
			if(pool->shutdown) {
				pthread_mutex_unlock(&(pool->qlock));
				pthread_exit(NULL);
			}
		}

		cur = pool->qhead;	

		pool->qsize--;		

		if(pool->qsize == 0) {
			pool->qhead = NULL;
			pool->qtail = NULL;
		}
		else {
			pool->qhead = cur->next;
		}

		if(pool->qsize == 0 && ! pool->shutdown) {
			
			pthread_cond_signal(&(pool->q_empty));
		}
		pthread_mutex_unlock(&(pool->qlock));
		(cur->routine) (cur->arg);  
		free(cur);						
	}
}

threadpool create_threadpool(int num_threads_in_pool) {
  _threadpool *pool;
	int i;

  
  if ((num_threads_in_pool <= 0) || (num_threads_in_pool > MAXT_IN_POOL))
    return NULL;

  pool = (_threadpool *) malloc(sizeof(_threadpool));
  if (pool == NULL) {
    fprintf(stderr, "Out of memory creating a new threadpool!\n");
    return NULL;
  }

  pool->threads = (pthread_t*) malloc (sizeof(pthread_t) * num_threads_in_pool);

  if(!pool->threads) {
    fprintf(stderr, "Out of memory creating a new threadpool!\n");
    return NULL;	
  }

  pool->num_threads = num_threads_in_pool; 
  pool->qsize = 0;
  pool->qhead = NULL;
  pool->qtail = NULL;
  pool->shutdown = 0;
  pool->dont_accept = 0;

  
  if(pthread_mutex_init(&pool->qlock,NULL)) {
    fprintf(stderr, "Mutex initiation error!\n");
	return NULL;
  }
  if(pthread_cond_init(&(pool->q_empty),NULL)) {
    fprintf(stderr, "CV initiation error!\n");	
	return NULL;
  }
  if(pthread_cond_init(&(pool->q_not_empty),NULL)) {
    fprintf(stderr, "CV initiation error!\n");	
	return NULL;
  }

  

  for (i = 0;i < num_threads_in_pool;i++) {
	  if(pthread_create(&(pool->threads[i]),NULL,do_work,pool)) {
	    fprintf(stderr, "Thread initiation error!\n");	
		return NULL;		
	  }
  }
  return (threadpool) pool;
}


void dispatch(threadpool from_me, dispatch_fn dispatch_to_here,
	      void *arg) {
  _threadpool *pool = (_threadpool *) from_me;
	work_t * cur;

	cur = (work_t*) malloc(sizeof(work_t));
	if(cur == NULL) {
		fprintf(stderr, "Out of memory creating a work struct!\n");
		return;	
	}

	cur->routine = dispatch_to_here;
	cur->arg = arg;
	cur->next = NULL;

	pthread_mutex_lock(&(pool->qlock));

	if(pool->dont_accept) {
		printf("Closed\n"); 
		free(cur); 
		return;
	}
	if(pool->qsize == 0) {
		pool->qhead = cur;  
		pool->qtail = cur;
		pthread_cond_signal(&(pool->q_not_empty));  
	} else {
		pool->qtail->next = cur;	
		pool->qtail = cur;			
	}
	pool->qsize++;
	pthread_mutex_unlock(&(pool->qlock));  
}

void destroy_threadpool(threadpool destroyme) {
	_threadpool *pool = (_threadpool *) destroyme;
	int i=0;
	void* nothing;
	
	pthread_mutex_lock(&(pool->qlock));
	for(;i < pool->num_threads;i++) {
		pthread_kill(pool->threads[i], SIGUSR1);
	}
	pthread_mutex_unlock(&(pool->qlock));

	
	for(i=0;i < pool->num_threads;i++) {

		pthread_join(pool->threads[i],&nothing);
	}

	free(pool->threads);

	pthread_mutex_destroy(&(pool->qlock));
	pthread_cond_destroy(&(pool->q_empty));
	pthread_cond_destroy(&(pool->q_not_empty));
	return;
}

