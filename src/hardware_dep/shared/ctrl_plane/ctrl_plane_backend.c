// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#include "ctrl_plane_backend.h"
#include "messages.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "threadpool.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include "sock_helpers.h"
#include "fifo.h"
#include <sys/select.h>
#include <unistd.h>

#include "backend.h"

#ifdef T4P4S_P4RT

#include "PI/proto/pi_server.h"
#include "p4rt/device_mgr.h"
extern device_mgr_t *dev_mgr_ptr;

#endif


volatile bool ctrl_is_initialized;


#define P4_BG_MEM_CELL_SIZE 2048
#define P4_BG_QUEUE_SIZE 1024

#define CTRL_INIT_TIMEOUT  10000

#define MIN(x,y) (((x) < (y)) ? (x) : (y))

typedef struct mem_cell_s {
    char* data;
    uint16_t length;
    struct mem_cell_s* next;
    struct mem_cell_s* prev;
} mem_cell_t;

typedef struct backend_st {
    char* msg_buffer; /* Slow memory allcoted for storing outgoing digest and incoming controll messages */
    mem_cell_t* unused_head;
    mem_cell_t* used_head;
    pthread_mutex_t memlock;
    pthread_cond_t mem_not_empty;     /*non empty and empty condidtion variables*/
    pthread_cond_t mem_empty;
    int shutdown;
    int dont_accept;
    threadpool tpool;
    int controller_fd;
    struct sockaddr_in controller_addr;  /*Assuming single controller, digest-receiver TODO: EXTEND IT FOR HANDLING MULTIPLE receivers and/or controllers */
    int controller_sock;
    fifo_t input_queue; /* one queue per controller should be needed */
    fifo_t output_queue; /* one queue per digest-receiver should be needed */
    p4_msg_callback cb;
} backend_t;

mem_cell_t* touch_mem_cell(backend_t* bgt);
void detouch_mem_cell(backend_t* bgt, mem_cell_t* cell);

void backend_processor(void* bg)
{
    backend_t* bgt = (backend_t*)bg;
    mem_cell_t* mem_cell;
    struct timeval tv;
    int rv;
    fd_set rfs;
    fd_set master;

    FD_ZERO(&master);
    FD_SET(bgt->controller_sock, &master);

    while (1)
    {
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        rfs = master;
        rv = select(bgt->controller_sock+1, &rfs, 0, 0, &tv);
        
        if (bgt->shutdown==1) break;
        if (rv==0) continue; /* timeout */

        if (FD_ISSET(bgt->controller_sock, &rfs))
        {
            mem_cell = touch_mem_cell(bgt);
                        while (NULL == mem_cell){
                                mem_cell = touch_mem_cell(bgt);
                                usleep(1000);
                        }

            if ((rv=read_p4_msg(bgt->controller_sock, mem_cell->data, mem_cell->length)) > 0)
            {
                fifo_add_msg( &(bgt->input_queue), (void*)mem_cell );
            } else if (rv==0) break; /*EOF TODO: RETRY !!!*/
        }
    }
    close(bgt->controller_sock);
}

void input_processor(void *bg)
{
    backend_t* bgt = (backend_t*)bg;
        mem_cell_t* mem_cell;
    int rval;

    while ( 1 )
    {
        fifo_wait( &(bgt->input_queue) );
        mem_cell = fifo_remove_msg(&(bgt->input_queue));

        if (bgt->shutdown==1) break;
        if (mem_cell==0) continue;

        rval = handle_p4_msg( mem_cell->data, mem_cell->length, bgt->cb );
#ifdef T4P4S_DEBUG
        if (rval != 0) {
            printf("[CTRL]  :::: rval = %d\n", rval);
        }
#endif

        detouch_mem_cell( bgt, mem_cell );
    }
}

void output_processor(void *bg)
{
    backend_t* bgt = (backend_t*)bg;
    mem_cell_t* mem_cell;

    while ( 1 )
    {
        fifo_wait( &(bgt->output_queue) );
        mem_cell = fifo_remove_msg(&(bgt->output_queue));

        if (bgt->shutdown==1) break;
        if (mem_cell==0) continue;

#ifdef T4P4S_P4RT
        dev_mgr_send_digest(dev_mgr_ptr, (struct p4_digest*)(mem_cell->data), 1);
#endif

#ifndef T4P4S_P4RT
        write_p4_msg(bgt->controller_sock, mem_cell->data, mem_cell->length);
#endif
        detouch_mem_cell( bgt, mem_cell );
    }
}

ctrl_plane_backend create_backend(int num_of_threads, int queue_size, char* controller_name, int controller_port, p4_msg_callback cb)
{
    backend_t *bg;
    mem_cell_t *tmp;
    int i;
    struct hostent *server;

    if ((num_of_threads<=0) || (queue_size<=0))
        return 0;

    bg = (backend_t*) malloc(sizeof(backend_t));
    if (bg == 0) {
        fprintf(stderr, "Out of memory creating a new ctrl_plane_backend!\n");
        return 0;
    }

    bg->msg_buffer = (char*) malloc(P4_BG_MEM_CELL_SIZE * queue_size);
    if (bg->msg_buffer == 0) {
                fprintf(stderr, "Out of memory creating a new msg_buffer!\n");
                return 0;
        }

    bg->unused_head = 0;
    bg->used_head = 0;

    for (i=0;i<queue_size;++i)
    {
        tmp = (mem_cell_t*) malloc(sizeof(mem_cell_t));
            if (tmp == 0) {
                    fprintf(stderr, "Out of memory creating a new mem cell list!\n");
                    return 0;
            }
        tmp->data = bg->msg_buffer + P4_BG_MEM_CELL_SIZE * i;
        tmp->length = P4_BG_MEM_CELL_SIZE;
        tmp->prev = 0;
        tmp->next = bg->unused_head;
        bg->unused_head = tmp;
    }

    bg->shutdown = 0;
    bg->dont_accept = 0;

    /* initialize mutex and condition variables. */
    if(pthread_mutex_init(&(bg->memlock),NULL)) {
        fprintf(stderr, "Mutex initiation error!\n");
        return 0;
    }
    if(pthread_cond_init(&(bg->mem_empty),NULL)) {
        fprintf(stderr, "CV initiation error!\n");
        return 0;
    }
    if(pthread_cond_init(&(bg->mem_not_empty),NULL)) {
        fprintf(stderr, "CV initiation error!\n");
        return 0;
    }

    bg->tpool = create_threadpool(num_of_threads);

    fifo_init(&(bg->input_queue));
    fifo_init(&(bg->output_queue));

    bg->controller_sock = socket( AF_INET, SOCK_STREAM, 0 );
        if( bg->controller_sock == -1 )
        {
                fprintf(stderr, "opening stream socket" );
                return 0;
        }

    /* Resolving controller's address */
    server = gethostbyname(controller_name);
    if (server == 0) {
            fprintf(stderr,"ERROR, Controller cannot be found, no such host: %s\n", controller_name);
            return 0;
    }
    
    memset((void*) &(bg->controller_addr), 0, sizeof(struct sockaddr_in));
    bg->controller_addr.sin_family = AF_INET;
    memcpy( (void*) &(bg->controller_addr.sin_addr), (void*) (server->h_addr), server->h_length);
    bg->controller_addr.sin_port = htons(controller_port);

    bg->cb = cb;

    return (ctrl_plane_backend) bg;
}

void launch_backend(ctrl_plane_backend bg)
{
    backend_t *bgt = (backend_t*) bg;
    ctrl_is_initialized = false;

#ifndef T4P4S_P4RT
    if( connect( bgt->controller_sock, (struct sockaddr *) &(bgt->controller_addr), sizeof(struct sockaddr_in) ) == -1 )
    {
        return;
    }  

    /* !!!!!!!!!!! Launch the client thread connecting to the controller  */

    dispatch(bgt->tpool, backend_processor, (void*)bgt);
    dispatch(bgt->tpool, input_processor, (void*)bgt);
#endif
    dispatch(bgt->tpool, output_processor, (void*)bgt);

#ifndef T4P4S_P4RT
    while (!ctrl_is_initialized) {
        usleep(CTRL_INIT_TIMEOUT);
    }
#endif
}

void stop_backend(ctrl_plane_backend bg)
{
    backend_t *bgt = (backend_t*) bg;
    bgt->shutdown = 1;
    
    destroy_threadpool(bgt->tpool); 
}


void destroy_backend(ctrl_plane_backend bg)
{
    backend_t *bgt = (backend_t*) bg;
    mem_cell_t *tmp;

    tmp = bgt->used_head;
    while (tmp!=0) {
        bgt->used_head = tmp->next;
        free(tmp);
        tmp = bgt->used_head;
    }

    tmp = bgt->unused_head;
    while (tmp!=0) {
        bgt->unused_head = tmp->next;
        free(tmp);
        tmp = bgt->unused_head;
    }
    
    free(bgt->msg_buffer);

    pthread_mutex_destroy(&(bgt->memlock));
    pthread_cond_destroy(&(bgt->mem_empty));
    pthread_cond_destroy(&(bgt->mem_not_empty));
    fifo_destroy( &(bgt->input_queue) );
    fifo_destroy( &(bgt->output_queue) );

    free(bgt);
}

mem_cell_t* touch_mem_cell(backend_t* bgt)
{
/*  backend_t* bgt = (backend_t*) bg;*/
    mem_cell_t* result = 0;
    pthread_mutex_lock(&(bgt->memlock));
    if (bgt->unused_head!=0) {
        result = bgt->unused_head;
        bgt->unused_head = result->next;
        if (bgt->unused_head!=0)
            bgt->unused_head->prev = 0;
        else
            pthread_cond_signal(&(bgt->mem_empty));

        if (bgt->used_head!=0) {
            bgt->used_head->prev = result;
            pthread_cond_signal(&(bgt->mem_not_empty));
        }
        result->next = bgt->used_head;
        bgt->used_head = result;
    }
    pthread_mutex_unlock(&(bgt->memlock));
    return result;
}

void detouch_mem_cell(backend_t* bgt, mem_cell_t* cell)
{
    pthread_mutex_lock(&(bgt->memlock));
    if (bgt->used_head!=cell)
        cell->prev->next = cell->next;
    else
        bgt->used_head = cell->next;

    if (cell->next!=0)
        cell->next->prev = cell->prev;
            if (bgt->unused_head!=0)
                    bgt->unused_head->prev = cell;
    cell->next = bgt->unused_head;
    cell->prev = 0;
    bgt->unused_head = cell;
    if (bgt->used_head==0)      
        pthread_cond_signal(&(bgt->mem_not_empty));
    pthread_mutex_unlock(&(bgt->memlock));
}

int send_digest(ctrl_plane_backend bg, ctrl_plane_digest d, uint32_t receiver_id)
{
    Digest_t* dt = (Digest_t*)d;
    backend_t* bgt = (backend_t*)bg;

    if (fifo_isfull( &(bgt->output_queue) )) {
	detouch_mem_cell(bgt, dt->mem_cell);
    }
    else {
        netconv_p4_header((struct p4_header*)(dt->ctrl_plane_digest));
        fifo_add_msg(&(bgt->output_queue), dt->mem_cell);
    }

    free(dt);
    return 0;
}

ctrl_plane_digest create_digest(ctrl_plane_backend bg, char* name)
{
    backend_t* bgt = (backend_t*) bg;
    Digest_t* dg = (Digest_t*) malloc( sizeof(Digest_t) );
    if (unlikely(dg==0))
    {
                fprintf(stderr, "Out of memory to a new ctrl_plane_digest message!\n");
                return 0;
    }
    
    dg->mem_cell = touch_mem_cell(bgt);
    if (unlikely(dg->mem_cell == 0))
    {
        //fprintf(stderr, "Out of memory pool - memcell cannot be assigned to a new ctrl_plane_digest message!\n");
        return 0;   
    }

    create_p4_header(dg->mem_cell->data, 0, dg->mem_cell->length);
    dg->ctrl_plane_digest = create_p4_digest(dg->mem_cell->data, 0, dg->mem_cell->length);

    if (unlikely(strlen( name ) > P4_MAX_FIELD_LIST_NAME_LEN-1))
    {
        fprintf(stderr, "Too long fieldname! The maximum length allowed is %d\n", (P4_MAX_FIELD_LIST_NAME_LEN-1));
        return 0;
    }

    strncpy( dg->ctrl_plane_digest->field_list_name, name, P4_MAX_FIELD_LIST_NAME_LEN );
    dg->ctrl_plane_digest->field_list_name[P4_MAX_FIELD_LIST_NAME_LEN-1] = '\0';

    return (ctrl_plane_digest) dg;
}


ctrl_plane_digest add_digest_field(ctrl_plane_digest d, void* value, uint32_t bitlength)
{
    Digest_t* dg = (Digest_t*) d;
    uint32_t bytelength = (bitlength-1)/8+1;

    struct p4_digest_field* dfield = add_p4_digest_field( dg->ctrl_plane_digest, dg->mem_cell->length );

    if (unlikely(bytelength>P4_MAX_FIELD_VALUE_LENGTH))
    {
        fprintf(stderr, "Too long value array! The maximum byte length allowed is %d\n", P4_MAX_FIELD_VALUE_LENGTH);
        return 0;
    }
    
    memcpy( dfield->value, value, MIN(bytelength, P4_MAX_FIELD_VALUE_LENGTH));
    dfield->length = bitlength;
    netconv_p4_digest_field(dfield);

    return d;
}
