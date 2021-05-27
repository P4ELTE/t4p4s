// Copyright 2019 Eotvos Lorand University, Budapest, Hungary
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

#if ASYNC_MODE != ASYNC_MODE_OFF

#include <unistd.h>

#include "dpdk_lib.h"
#include "gen_include.h"
#include "dpdkx_crypto.h"
#include "rte_errno.h"

// -----------------------------------------------------------------------------
// GLOBALS

extern struct rte_mempool *crypto_task_pool;
#if ASYNC_MODE == ASYNC_MODE_PD
    struct rte_mempool *pd_pool;
#endif


#if ASYNC_MODE == ASYNC_MODE_CONTEXT
    struct rte_mempool *context_pool;
    struct rte_ring    *context_free_command_ring;
#endif

// -----------------------------------------------------------------------------
// INTERFACE

void async_init_storage();
void async_handle_packet(int port_id, unsigned queue_idx, unsigned pkt_idx, packet_handler_t handler_function, LCPARAMS);
void main_loop_async(LCPARAMS);
void main_loop_fake_crypto(LCPARAMS);
void do_crypto_task(packet_descriptor_t* pd, enum crypto_task_type op);

// -----------------------------------------------------------------------------
// DEBUG

extern struct lcore_conf   lcore_conf[RTE_MAX_LCORE];

#if ASYNC_MODE == ASYNC_MODE_PD
    #include <setjmp.h>

    void jump_to_main_loop(packet_descriptor_t* pd){
        if(pd->program_restore_phase == 0){
            debug(" " T4LIT(<<<<,async) " Longjump to the " T4LIT(main loop,async) "\n");
            longjmp(lcore_conf[rte_lcore_id()].mainLoopJumpPoint,1);
        }else{
            debug(" " T4LIT(<<<<,async) " Longjump to the " T4LIT(async loop,async) "\n");
            longjmp(lcore_conf[rte_lcore_id()].asyncLoopJumpPoint,1);
        }
    }
#endif

// -----------------------------------------------------------------------------
// EXTERNS

extern struct lcore_conf   lcore_conf[RTE_MAX_LCORE];

// defined in dataplane.c
void init_headers(packet_descriptor_t* pd, lookup_table_t** tables);
void reset_headers(packet_descriptor_t* pd, lookup_table_t** tables);
void parse_packet(packet_descriptor_t* pd, lookup_table_t** tables, parser_state_t* pstate);
void emit_packet(packet_descriptor_t* pd, lookup_table_t** tables, parser_state_t* pstate);
void control_DeparserImpl(packet_descriptor_t* pd, lookup_table_t** tables, parser_state_t* pstate);
extern void free_packet(LCPARAMS);

// -----------------------------------------------------------------------------
// SERIALIZATION AND DESERIALIZATION

extern void reset_pd(packet_descriptor_t *pd);
extern void do_handle_packet(LCPARAMS, int portid, unsigned queue_idx, unsigned pkt_idx);
void main_loop_post_rx(struct lcore_data* lcdata);
void main_loop_post_single_rx(struct lcore_data* lcdata, bool got_packet);

static void resume_packet_handling(struct rte_mbuf *mbuf, struct lcore_data* lcdata, packet_descriptor_t *pd)
{
    debug_mbuf(mbuf, "Data after async function: ");

    // Extracting extra content from the mbuf

    int packet_length = *(rte_pktmbuf_mtod(mbuf, uint32_t*));
    rte_pktmbuf_adj(mbuf, sizeof(uint32_t));

    #if ASYNC_MODE == ASYNC_MODE_PD
        debug_mbuf(mbuf, "Data after removing packet length: ");
        debug("Loaded packet length: %d\n",packet_length);
    #endif

    #if ASYNC_MODE == ASYNC_MODE_CONTEXT
        void* context = *(rte_pktmbuf_mtod(mbuf, void**));
        rte_pktmbuf_adj(mbuf, sizeof(void*));
    #elif ASYNC_MODE == ASYNC_MODE_PD
        packet_descriptor_t* async_pds_id = *(rte_pktmbuf_mtod(mbuf, packet_descriptor_t**));
        rte_pktmbuf_adj(mbuf, sizeof(packet_descriptor_t*));
        //debug("Loading from async PD store! id is %d\n",async_pds_id);
        *pd = *async_pds_id;
        if(pd->program_restore_phase == 0) {
            rte_mempool_put_bulk(pd_pool, (void **) &async_pds_id, 1);
        }
    #endif

    // Resetting the pd
    init_headers(pd, 0);
    reset_headers(pd, 0);
    reset_pd(pd);

    pd->wrapper = mbuf;
    pd->data = rte_pktmbuf_mtod(pd->wrapper, uint8_t*);

    pd->wrapper->pkt_len = packet_length;

    #if ASYNC_MODE == ASYNC_MODE_CONTEXT
        pd->context = context;

        debug("   " T4LIT(<<,async) " Swapping to packet context " T4LIT(%p,async) "\n", context);
        swapcontext(&lcdata->conf->main_loop_context, context);
        debug("   " T4LIT(>>,async) " Swapped back to " T4LIT(main context,async) "\n");
    #elif ASYNC_MODE == ASYNC_MODE_PD
        // parse
        debug("-------------------------\n")
        debug("PD Restoring\n")
        reset_pd(pd);
        parse_packet(pd, 0, 0);
        pd->program_restore_phase += 1;
        do_handle_packet(lcdata, pd, pd->port_id, pd->queue_idx, pd->pkt_idx);
    #endif
}



extern void create_crypto_op(struct crypto_task **op_out, packet_descriptor_t* pd, enum crypto_task_type op_type, int offset, void* extraInformationForAsyncHandling);

void enqueue_packet_for_async(packet_descriptor_t* pd, enum crypto_task_type op_type, void* extraInformationForAsyncHandling)
{
    struct crypto_task *op;
    create_crypto_op(&op,pd,op_type,0,extraInformationForAsyncHandling);

    rte_ring_enqueue(lcore_conf[rte_lcore_id()].async_queue, op);
    debug_mbuf(op->data, "   :: Enqueued for async");
}

// -----------------------------------------------------------------------------
// CALLBACKS

void async_init_storage()
{
    #if ASYNC_MODE == ASYNC_MODE_CONTEXT
        context_pool = rte_mempool_create("context_pool", (unsigned)CRYPTO_CONTEXT_POOL_SIZE*1024-1, sizeof(ucontext_t) + CONTEXT_STACKSIZE, MEMPOOL_CACHE_SIZE, 0, NULL, NULL, NULL, NULL, 0, 0);
        if (context_pool == NULL) rte_exit(EXIT_FAILURE, "Cannot create context pool\n");

        context_free_command_ring = rte_ring_create("context_ring", (unsigned)CRYPTO_RING_SIZE*1024, SOCKET_ID_ANY, 0 /*RING_F_SP_ENQ | RING_F_SC_DEQ */);
        if (context_free_command_ring == NULL) rte_exit(EXIT_FAILURE, "Cannot create context ring!\n");
    #endif

    #if ASYNC_MODE == ASYNC_MODE_PD
        pd_pool = rte_mempool_create("pd_pool", (unsigned)CRYPTO_CONTEXT_POOL_SIZE*1024-1, sizeof(packet_descriptor_t), MEMPOOL_CACHE_SIZE, 0, NULL, NULL, NULL, NULL, 0, 0);
        if (pd_pool == NULL) rte_exit(EXIT_FAILURE, "Cannot create pd pool\n");
    #endif
}


void init_async_data(struct lcore_data *data){
    data->conf->crypto_pool = crypto_pool;
    char str[15];
    sprintf(str, "async_queue_%d", rte_lcore_id());
    data->conf->async_queue = rte_ring_create(str, (unsigned)1024, SOCKET_ID_ANY, RING_F_SP_ENQ | RING_F_SC_DEQ);

    char rxName[32];
    char txName[32];
    sprintf(rxName,"fake_crypto_rx_ring_%d",rte_lcore_id());
    sprintf(txName,"fake_crypto_tx_ring_%d",rte_lcore_id());
    data->conf->fake_crypto_rx = rte_ring_create(rxName, (unsigned) CRYPTO_RING_SIZE*1024, SOCKET_ID_ANY,
                                                   0 /*RING_F_SP_ENQ | RING_F_SC_DEQ */);
    data->conf->fake_crypto_tx = rte_ring_create(txName, (unsigned) CRYPTO_RING_SIZE*1024, SOCKET_ID_ANY,
                                                   0 /*RING_F_SP_ENQ | RING_F_SC_DEQ */);

    #ifdef DEBUG__CRYPTO_EVERY_N
        data->conf->crypto_every_n_counter = -1;
    #endif
    COUNTER_INIT(data->conf->async_drop_counter);
}

void async_handle_packet(int port_id, unsigned queue_idx, unsigned pkt_idx, packet_handler_t handler_function, LCPARAMS)
{
    pd->port_id = port_id;
    pd->queue_idx = queue_idx;
    pd->pkt_idx = pkt_idx;

    uint8_t dropped = 0;
    COUNTER_ECHO(lcdata->conf->async_drop_counter,"   :: Dropped async: %d\n");

    #if ASYNC_MODE == ASYNC_MODE_CONTEXT
        ucontext_t *context;
        if(rte_mempool_get(context_pool, (void**)&context) != 0) {
            dropped = 1;
            free_packet(LCPARAMS_IN);
            pd->context = NULL;
        }else{
            COUNTER_STEP(lcdata->conf->async_packet);
            context->uc_stack.ss_sp = (ucontext_t*)context + 1; // the stack is supposed to be placed right after the context description
            context->uc_stack.ss_size = CONTEXT_STACKSIZE;
            context->uc_stack.ss_flags = 0;
            sigemptyset(&context->uc_sigmask);
            pd->context = context;

            getcontext(context);
            context->uc_link = &lcdata->conf->main_loop_context;
            makecontext(context, (packet_handler_noparams_t)handler_function, 5, port_id, queue_idx, pkt_idx, LCPARAMS_IN);
            debug("   " T4LIT(<<,async) " Swapping to packet context " T4LIT(%p,async) "\n", context);
            swapcontext(&lcdata->conf->main_loop_context, context);
            debug("   " T4LIT(>>,async) " Swapped back to " T4LIT(main context,async) "\n");
        }
    #elif ASYNC_MODE == ASYNC_MODE_PD
        packet_handler_t handler_fun = handler_function;
        packet_descriptor_t *pd_store;

        int ret = rte_mempool_get(pd_pool, (void**)(&pd_store));
        if(ret != 0){
            dropped = 1;
            free_packet(LCPARAMS_IN);
            pd->context = NULL;
        }else{
            COUNTER_STEP(lcdata->conf->async_packet);
            pd->context = pd_store;

            if(setjmp(lcore_conf[rte_lcore_id()].mainLoopJumpPoint) == 0) {
                handler_fun(port_id, queue_idx, pkt_idx, LCPARAMS_IN);
            }
        }
    #endif


    if(dropped > 0){
        COUNTER_STEP(lcore_conf[rte_lcore_id()].async_drop_counter);
    }
}

void do_crypto_task(packet_descriptor_t* pd, enum crypto_task_type op)
{
    void* extraInformationForAsyncHandling = NULL;

    #if ASYNC_MODE == ASYNC_MODE_CONTEXT
        if(pd->context == NULL) return;

        extraInformationForAsyncHandling = pd->context;

        // saving standard metadata into context
        int metadata_length = pd->headers[1].length;
        uint8_t standard_metadata[metadata_length];
        memcpy(standard_metadata,
               pd->headers[1].pointer,
               metadata_length);
    #elif ASYNC_MODE == ASYNC_MODE_PD
        if(pd->context == NULL) return;
        extraInformationForAsyncHandling = pd->context;
    #endif

    // deparse
    control_DeparserImpl(pd, 0, 0);
    emit_packet(pd, 0, 0);

    // enqueue mbuf to async operation buffer
    enqueue_packet_for_async(pd, op, extraInformationForAsyncHandling);


    #if ASYNC_MODE == ASYNC_MODE_CONTEXT
        void* context = extraInformationForAsyncHandling;
        // suspend processing of packet and go back to the main context
        debug("   " T4LIT(<<,async) " Swapping to " T4LIT(main context,async) "\n");
        swapcontext(context, &lcore_conf[rte_lcore_id()].main_loop_context);
        debug("   " T4LIT(>>,async) " Swapped back to packet context " T4LIT(%p,async) "\n", context);
        reset_pd(pd);
        parse_packet(pd, 0, 0);
        // restoring standard metadata from context
        memcpy(pd->headers[1].pointer,
               standard_metadata,
               metadata_length);
    #elif ASYNC_MODE == ASYNC_MODE_PD
        //init_headers(pd, 0);
        //reset_headers(pd, 0);
        reset_pd(pd);
        parse_packet(pd, 0, 0);
        jump_to_main_loop(pd);
    #endif
}
#if ASYNC_MODE == ASYNC_MODE_CONTEXT
    ucontext_t* cs[RTE_MAX_LCORE][CRYPTO_BURST_SIZE];
#endif

extern struct crypto_task *crypto_tasks[RTE_MAX_LCORE][CRYPTO_BURST_SIZE];
extern struct rte_crypto_op* enqueued_ops[RTE_MAX_LCORE][CRYPTO_BURST_SIZE];
extern struct rte_crypto_op* dequeued_ops[RTE_MAX_LCORE][CRYPTO_BURST_SIZE];

static inline void
wait_for_cycles(uint64_t cycles)
{
    uint64_t now = rte_get_tsc_cycles();
    uint64_t then = now;
    while((now - then) < cycles)
        now = rte_get_tsc_cycles();
}
void main_loop_fake_crypto(LCPARAMS){
    unsigned lcore_id = rte_lcore_id();
    for(int a=0;a<rte_lcore_count();a++){
        if(lcore_conf[a].fake_crypto_rx != NULL){
            unsigned int n = rte_ring_dequeue_burst(lcore_conf[a].fake_crypto_rx, (void*)enqueued_ops[lcore_id], CRYPTO_BURST_SIZE, NULL);
            if (n>0){
                debug("---------------- Received from %d %d packet\n",a,n);
                #if CRYPTO_NODE_MODE == CRYPTO_NODE_MODE_OPENSSL
                    rte_cryptodev_enqueue_burst(cdev_id, lcore_id, enqueued_ops[lcore_id], n);
                    int already_dequed_ops = 0;
                    while(already_dequed_ops < n){
                        already_dequed_ops += rte_cryptodev_dequeue_burst(cdev_id, lcore_id, dequeued_ops[lcore_id], n - already_dequed_ops);
                    }
                #elif CRYPTO_NODE_MODE == CRYPTO_NODE_MODE_FAKE
                    wait_for_cycles(FAKE_CRYPTO_SLEEP_MULTIPLIER*n);
                #endif

                for(int b=0;b<n;b++){
                    enqueued_ops[lcore_id][b]->status = RTE_CRYPTO_OP_STATUS_SUCCESS;
                }
                if (rte_ring_enqueue_burst(lcore_conf[a].fake_crypto_tx, (void*)enqueued_ops[lcore_id], n, NULL) <= 0){
                    debug(T4LIT(Enqueing from fake crypto core failed,error) "\n");
                }
            }
        }
    }
}

uint64_t main_loop_async_work_timer = 0;
uint64_t main_loop_async_tick_timer = 0;

void main_loop_async(LCPARAMS)
{
    ONE_PER_SEC(main_loop_async_tick_timer){
        debug("Main loop async - async_size:%d, pending: %d\n", rte_ring_count(lcdata->conf->async_queue),lcdata->conf->pending_crypto);
    }

    unsigned lcore_id = rte_lcore_id();
    unsigned n, i;
    #if ASYNC_MODE == ASYNC_MODE_CONTEXT
        if(rte_ring_count(context_free_command_ring) > CRYPTO_BURST_SIZE)
            {
                n = rte_ring_dequeue_burst(context_free_command_ring, (void**)cs[lcore_id], CRYPTO_BURST_SIZE, NULL);
                for(i = 0; i < n; i++)
                    debug(T4LIT(Packet context %p is being freed up.,warning) "\n", cs[lcore_id][i]);
                rte_mempool_put_bulk(context_pool, (void**)cs[lcore_id], n);
            }
    #endif

    if(CRYPTO_DEVICE_AVAILABLE)
    {
        if(rte_ring_count(lcdata->conf->async_queue) >= CRYPTO_BURST_SIZE)
        {
            n = rte_ring_dequeue_burst(lcdata->conf->async_queue, (void**)crypto_tasks[lcore_id], CRYPTO_BURST_SIZE, NULL);
            if(n > 0)
            {
                if (rte_crypto_op_bulk_alloc(lcdata->conf->crypto_pool, RTE_CRYPTO_OP_TYPE_SYMMETRIC, enqueued_ops[lcore_id], n) == 0)
                    rte_exit(EXIT_FAILURE, "Not enough crypto operations available\n");
                for(i = 0; i < n; i++)
                    crypto_task_to_crypto_op(crypto_tasks[lcore_id][i], enqueued_ops[lcore_id][i]);
                rte_mempool_put_bulk(crypto_task_pool, (void**)crypto_tasks[lcore_id], n);
                #ifdef START_CRYPTO_NODE
                    lcdata->conf->pending_crypto += rte_ring_enqueue_burst(lcore_conf[lcore_id].fake_crypto_rx, (void**)enqueued_ops[lcore_id], n, NULL);
                #else
                    lcdata->conf->pending_crypto += rte_cryptodev_enqueue_burst(cdev_id, lcore_id, enqueued_ops[lcore_id], n);
                #endif
            }
        }

        if(lcdata->conf->pending_crypto >= CRYPTO_BURST_SIZE)
        {
            #ifdef START_CRYPTO_NODE
                n = rte_ring_dequeue_burst(lcore_conf[lcore_id].fake_crypto_tx, (void**)dequeued_ops[lcore_id], CRYPTO_BURST_SIZE, NULL);
            #else
                n = rte_cryptodev_dequeue_burst(cdev_id, lcore_id, dequeued_ops[lcore_id], CRYPTO_BURST_SIZE);
            #endif
            ONE_PER_SEC(main_loop_async_work_timer){
                debug("Async work function %d\n",lcdata->conf->pending_crypto);
            }
            if(n > 0){
                for (i = 0; i < n; i++)
                {
                    if (dequeued_ops[lcore_id][i]->status != RTE_CRYPTO_OP_STATUS_SUCCESS){
                        rte_exit(EXIT_FAILURE, "Some operations were not processed correctly");
                    }
                    else{
                        dequeued_ops[lcore_id][i]->status = RTE_CRYPTO_OP_STATUS_SUCCESS;
                        #if ASYNC_MODE == ASYNC_MODE_PD
                            if(setjmp(lcore_conf[rte_lcore_id()].asyncLoopJumpPoint) == 0) {
                                resume_packet_handling(dequeued_ops[lcore_id][i]->sym->m_src, lcdata, pd);
                            }
                        #else
                            resume_packet_handling(dequeued_ops[lcore_id][i]->sym->m_src, lcdata, pd);
                        #endif
                    }
                }
                rte_mempool_put_bulk(lcdata->conf->crypto_pool, (void **)dequeued_ops[lcore_id], n);
                lcdata->conf->pending_crypto -= n;
            }
        }
    }
}

#endif