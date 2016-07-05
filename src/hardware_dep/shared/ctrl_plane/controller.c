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
#include "controller.h"
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "sock_helpers.h"
#include "fifo.h"
#include <sys/select.h>
#include "threadpool.h"

typedef struct msg_buf_st
{
	char data[2048]; /* todo */
	int length;
} msg_buf_t;

typedef struct controller_st
{
        threadpool tpool;
        fifo_t input_queue;
        fifo_t output_queue;
        int controller_fd;
	digest_handler dh;
	initialize init;
	int port;
} controller_t;

typedef struct threadinfo_st {
	controller_t* ct;
	int sock_fd;
} threadinfo_t;


void input_processor(void *t)
{
	threadinfo_t* ti = (threadinfo_t*)t;
        controller_t* ct = ti->ct;
        msg_buf_t* mem_cell;

        while ( 1 )
        {
                fifo_wait( &(ct->input_queue) );
                mem_cell = fifo_remove_msg(&(ct->input_queue));

                if (mem_cell==0) continue;

                ct->dh( mem_cell->data );


                free( mem_cell );
        }
}

void output_processor(void *t)
{
	threadinfo_t* ti = (threadinfo_t*)t;
        controller_t* ct = ti->ct;
        msg_buf_t* mem_cell;

        while ( 1 )
        {
                fifo_wait( &(ct->output_queue) );
                mem_cell = fifo_remove_msg(&(ct->output_queue));

                if (mem_cell==0) continue;
                write_p4_msg(ti->sock_fd, mem_cell->data, mem_cell->length);
		free ( mem_cell );
        }
}

controller create_controller_with_init(uint16_t port, int number_of_threads, digest_handler dh, initialize init)
{
	controller_t* ct;
	struct sockaddr_in server;
	/*struct sockaddr_in client;*/

	if ((port<=0) || (number_of_threads<=0))
		return 0;

	ct = (controller_t*) malloc(sizeof(controller_t));
	
	if (ct==0) 
		return 0;

    ct->tpool = create_threadpool(3);/*number_of_threads);*/

    fifo_init(&(ct->input_queue));
    fifo_init(&(ct->output_queue));
	ct->dh = dh;
	ct->init = init;

	ct->controller_fd = socket( PF_INET, SOCK_STREAM, 0 );

	/* Server's information*/
	server.sin_family = AF_INET;	
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);
	ct->port = port;

	/* binding the addressing information to the listen socket*/
	bind( ct->controller_fd, (struct sockaddr *) &server, sizeof server );

	listen( ct->controller_fd, 5 );
	
	return (controller)ct;

}



controller create_controller(uint16_t port, int number_of_threads, digest_handler dh)
{
	controller_t* ct;
	struct sockaddr_in server;
	/*struct sockaddr_in client;*/

	if ((port<=0) || (number_of_threads<=0))
		return 0;

	ct = (controller_t*) malloc(sizeof(controller_t));
	
	if (ct==0) 
		return 0;

        ct->tpool = create_threadpool(3);/*number_of_threads);*/

        fifo_init(&(ct->input_queue));
        fifo_init(&(ct->output_queue));
	ct->dh = dh;
	ct->init = 0;

	ct->controller_fd = socket( PF_INET, SOCK_STREAM, 0 );

	/* Server's information*/
	server.sin_family = AF_INET;	
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);
	ct->port = port;

	/* binding the addressing information to the listen socket*/
	bind( ct->controller_fd, (struct sockaddr *) &server, sizeof server );

	listen( ct->controller_fd, 5 );
	
	return (controller)ct;
}

void destroy_controller(controller c)
{
	controller_t* ct = (controller_t*)c;
	destroy_threadpool(ct->tpool);
        fifo_destroy( &(ct->input_queue) );
        fifo_destroy( &(ct->output_queue) );
	close(ct->controller_fd);
	free(ct);
}

void execute_controller(controller c)
{
	controller_t* ct = (controller_t*)c;
	struct sockaddr_in client;
	socklen_t length;
	threadinfo_t* ti;
	int conn;
	int i;
	int rv;
	fd_set master;
	fd_set readfds;
	int maxfds = ct->controller_fd;
	msg_buf_t*  mem_cell;

	FD_ZERO(&master);
	FD_SET(ct->controller_fd, &master);

	while (1) { /*TODO - replace this stupid implementation*/

		readfds = master;

		select(maxfds+1, &readfds, 0,0,0);

		for (i=0;i<maxfds+1;++i)
		{
			if (FD_ISSET(i, &readfds)) {
				if (i==ct->controller_fd) {
					if ((conn = accept( ct->controller_fd, (struct sockaddr *) &client, &length )) < 0 )
			                        break;

					printf("New device connected...\n");
			        ti = (threadinfo_t*) malloc(sizeof(threadinfo_t));
			        ti->ct = ct;
			        ti->sock_fd = conn;

					dispatch(ct->tpool, output_processor, (void*)ti);
					sleep(1);
				    dispatch(ct->tpool, input_processor, (void*)ti);
					FD_SET(conn, &master);
					maxfds = conn>maxfds?conn:maxfds;
					printf("Initialize switch\n");
					if (ct->init!=0)
						ct->init();
				} else {
					mem_cell = (msg_buf_t*)malloc(sizeof(msg_buf_t));
					mem_cell->length = 2048;
					if ((rv=read_p4_msg(i, mem_cell->data, mem_cell->length)) > 0)
                        		{
               			                 fifo_add_msg( &(ct->input_queue), (void*)mem_cell );
		                        } else if (rv==0) {
						FD_CLR(i, &master);
						close(i);
						free(mem_cell);
						break;
					}	
				}
			} 
		}			
	}
	
	
}

int send_p4_msg(controller c, char* buffer, int length)
{
	controller_t* ct = (controller_t*)c;
	msg_buf_t* mem_cell;
	if (length>2048)
		return 0;

	mem_cell = (msg_buf_t*)malloc(sizeof(msg_buf_t));

	memcpy(mem_cell->data, buffer, length);
	mem_cell->length = length;
	if( fifo_add_msg( &(ct->output_queue), (void*)mem_cell )==0)
		printf("ERR!!\n");

	return 0;
}

