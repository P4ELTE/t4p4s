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

#include "dpdk_lib.h"
#include "gen_include.h"
#include "dpdkx_crypto.h"
#include "rte_errno.h"

// -----------------------------------------------------------------------------
// GLOBALS

struct rte_mempool *async_pool;
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
void async_handle_packet(LCPARAMS, int port_id, unsigned queue_idx, unsigned pkt_idx, void (*handler_function)(LCPARAMS, int port_id, unsigned queue_idx, unsigned pkt_idx));
void main_loop_async(LCPARAMS);
void main_loop_fake_crypto(LCPARAMS);
void do_async_op(packet_descriptor_t* pd, enum async_op_type op);
void do_blocking_sync_op(packet_descriptor_t* pd, enum async_op_type op);

// -----------------------------------------------------------------------------
// DEBUG

#define DBG_CONTEXT_SWAP_TO_MAIN        debug(T4LIT(Swapping to main context...,warning) "\n");
#define DBG_CONTEXT_SWAP_TO_PACKET(ctx) debug(T4LIT(Swapping to packet context (%p)...,warning) "\n", ctx);

extern struct lcore_conf   lcore_conf[RTE_MAX_LCORE];

#if ASYNC_MODE == ASYNC_MODE_PD
    #include <setjmp.h>

    void jump_to_main_loop(packet_descriptor_t* pd){
        if(pd->program_state == 0){
            debug("========================================================= Jump to the main loop\n");
            longjmp(lcore_conf[rte_lcore_id()].mainLoopJumpPoint,1);
        }else{
            debug("========================================================= Jump to the async loop\n");
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

static void reset_pd(packet_descriptor_t *pd)
{
    pd->parsed_length = 0;
    if(pd->wrapper == 0){
        pd->payload_length = 0;
    }else{
        pd->payload_length = rte_pktmbuf_pkt_len(pd->wrapper) - pd->parsed_length;
    }
    pd->emit_hdrinst_count = 0;
    pd->is_emit_reordering = false;
}

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
        if(pd->program_state == 1) {
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

        DBG_CONTEXT_SWAP_TO_PACKET(context)
        swapcontext(&lcdata->conf->main_loop_context, context);
        debug("Swapped back to main context.\n");

    #elif ASYNC_MODE == ASYNC_MODE_PD
        // parse
        debug("-------------------------\n")
        debug("PD Restoring\n")
        reset_pd(pd);
        parse_packet(pd, 0, 0);
        pd->program_state += 1;
        do_handle_packet(lcdata, pd, pd->port_id, pd->queue_idx, pd->pkt_idx);
    #endif
}




void create_crypto_op(struct async_op **op_out, packet_descriptor_t* pd, enum async_op_type op_type, void* extraInformationForAsyncHandling){
    unsigned encryption_offset = 0;//14; // TODO

    int ret;
    ret = rte_mempool_get(async_pool, (void**)op_out);
    if(ret < 0){
        rte_exit(EXIT_FAILURE, "Mempool get failed!\n");
        //TODO: it should be a packet drop, not total fail
    }
    struct async_op *op = *op_out;
    op->op = op_type;
    op->data = pd->wrapper;

    uint32_t packet_length = op->data->pkt_len;
    int encrypted_length = packet_length - encryption_offset;
    int extra_length = 0;
    debug_mbuf(op->data, "Create crypto op, packet: ");

    #if ASYNC_MODE == ASYNC_MODE_CONTEXT
        if(extraInformationForAsyncHandling != NULL){
            void* context = extraInformationForAsyncHandling;
            rte_pktmbuf_prepend(op->data, sizeof(void*));
            *(rte_pktmbuf_mtod(op->data, void**)) = context;
            extra_length += sizeof(void*);
        }
    #elif ASYNC_MODE == ASYNC_MODE_PD
        packet_descriptor_t *store_pd = extraInformationForAsyncHandling;
        *store_pd = *pd;

        rte_pktmbuf_prepend(op->data, sizeof(packet_descriptor_t*));
        //debug("Saving to Async PD store! id is %d\n",store_pd);
        *(rte_pktmbuf_mtod(op->data, packet_descriptor_t**)) = store_pd;
        extra_length += sizeof(void**);
    #endif

    rte_pktmbuf_prepend(op->data, sizeof(uint32_t));
    *(rte_pktmbuf_mtod(op->data, uint32_t*)) = packet_length;
    extra_length += sizeof(int);
    debug("Saved packet length: %d\n",packet_length);

    debug_mbuf(op->data, "Prepared for encryption (added extra informations):");

    op->offset = extra_length + encryption_offset;
    // This is extremely important, believe me. The pkt_len has to be a multiple of the cipher block size, otherwise the crypto device won't do the operation on the mbuf.
    if(encrypted_length%16 != 0) rte_pktmbuf_append(op->data, 16-encrypted_length%16);
}

void enqueue_packet_for_async(packet_descriptor_t* pd, enum async_op_type op_type, void* extraInformationForAsyncHandling)
{
    struct async_op *op;
    create_crypto_op(&op,pd,op_type,extraInformationForAsyncHandling);

    rte_ring_enqueue(lcore_conf[rte_lcore_id()].async_queue, op);
    debug_mbuf(op->data, "Enqueued for async");
}

// -----------------------------------------------------------------------------
// CALLBACKS

void async_init_storage()
{
    async_pool = rte_mempool_create("async_pool", (unsigned)16*1024-1, sizeof(struct async_op), MEMPOOL_CACHE_SIZE, 0, NULL, NULL, NULL, NULL, 0, 0);
    if (async_pool == NULL) {
        switch(rte_errno){
            case E_RTE_NO_CONFIG:  rte_exit(EXIT_FAILURE, "Cannot create async op pool - function could not get pointer to rte_config structure\n"); break;
            case E_RTE_SECONDARY:  rte_exit(EXIT_FAILURE, "Cannot create async op pool - function was called from a secondary process instance\n"); break;
            case EINVAL:  rte_exit(EXIT_FAILURE, "Cannot create async op pool - cache size provided is too large\n"); break;
            case ENOSPC:  rte_exit(EXIT_FAILURE, "Cannot create async op pool - the maximum number of memzones has already been allocated\n"); break;
            case EEXIST:  rte_exit(EXIT_FAILURE, "Cannot create async op pool - a memzone with the same name already exists\n"); break;
            case ENOMEM:  rte_exit(EXIT_FAILURE, "Cannot create async op pool - no appropriate memory area found in which to create memzone\n"); break;
            default:  rte_exit(EXIT_FAILURE, "Cannot create async op pool - Unknown error %d\n",rte_errno); break;
        }
    }

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

void async_handle_packet(LCPARAMS, int port_id, unsigned queue_idx, unsigned pkt_idx, void (*handler_function)(LCPARAMS, int port_id, unsigned queue_idx, unsigned pkt_idx))
{
    pd->port_id = port_id;
    pd->queue_idx = queue_idx;
    pd->pkt_idx = pkt_idx;

    uint8_t dropped = 0;
    COUNTER_ECHO(lcdata->conf->async_drop_counter,"dropped async: %d\n");

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
            debug("Packet being handled, context reference is %p\n", context);

            getcontext(context);
            context->uc_link = &lcdata->conf->main_loop_context;
            makecontext(context, handler_function, 5, LCPARAMS_IN, port_id, queue_idx, pkt_idx);
            DBG_CONTEXT_SWAP_TO_PACKET(context)
            swapcontext(&lcdata->conf->main_loop_context, context);
            debug("Swapped back to main context.\n");
        }
    #elif ASYNC_MODE == ASYNC_MODE_PD
        void (*handler_function_with_params)(LCPARAMS, int port_id, unsigned queue_idx, unsigned pkt_idx) = (void (*)(LCPARAMS, int port_id, unsigned queue_idx, unsigned pkt_idx))handler_function;
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
                handler_function_with_params(LCPARAMS_IN, port_id, queue_idx, pkt_idx);
            }
        }
    #endif


    if(dropped > 0){
        COUNTER_STEP(lcore_conf[rte_lcore_id()].async_drop_counter);
    }
}

void do_async_op(packet_descriptor_t* pd, enum async_op_type op)
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
        DBG_CONTEXT_SWAP_TO_MAIN
        swapcontext(context, &lcore_conf[rte_lcore_id()].main_loop_context);
        debug("Swapped back to packet context %p.\n", context);
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

struct async_op *async_ops[RTE_MAX_LCORE][CRYPTO_BURST_SIZE];
struct rte_crypto_op* enqueued_ops[RTE_MAX_LCORE][CRYPTO_BURST_SIZE];
struct rte_crypto_op* dequeued_ops[RTE_MAX_LCORE][CRYPTO_BURST_SIZE];

#include <unistd.h>

void do_blocking_sync_op(packet_descriptor_t* pd, enum async_op_type op){
    unsigned int lcore_id = rte_lcore_id();

    control_DeparserImpl(pd, 0, 0);
    emit_packet(pd, 0, 0);

    create_crypto_op(async_ops[lcore_id],pd,op,NULL);
    if (rte_crypto_op_bulk_alloc(lcore_conf[lcore_id].crypto_pool, RTE_CRYPTO_OP_TYPE_SYMMETRIC, enqueued_ops[lcore_id], 1) == 0){
        rte_exit(EXIT_FAILURE, "Not enough crypto operations available\n");
    }
    async_op_to_crypto_op(async_ops[lcore_id][0], enqueued_ops[lcore_id][0]);
    rte_mempool_put_bulk(async_pool, (void **) async_ops[lcore_id], 1);

    #ifdef START_CRYPTO_NODE
        if (rte_ring_enqueue_burst(lcore_conf[lcore_id].fake_crypto_rx, (void**)enqueued_ops[lcore_id], 1, NULL) <= 0){
            debug(T4LIT(Enqueing ops in blocking sync op failed... skipping encryption,error) "\n");
            return;
        }
        while(rte_ring_dequeue_burst(lcore_conf[lcore_id].fake_crypto_tx, (void**)dequeued_ops[lcore_id], 1, NULL) == 0);
    #else
        if(rte_cryptodev_enqueue_burst(cdev_id, lcore_id,enqueued_ops[lcore_id], 1) <= 0){
            debug(T4LIT(Enqueing ops in blocking sync op failed... skipping encryption,error) "\n");
            return;
        }
        while(rte_cryptodev_dequeue_burst(cdev_id, lcore_id, dequeued_ops[lcore_id], 1) == 0);
    #endif
    struct rte_mbuf *mbuf = dequeued_ops[lcore_id][0]->sym->m_src;
    int packet_length = *(rte_pktmbuf_mtod(mbuf, int*));

    rte_pktmbuf_adj(mbuf, sizeof(int));
    pd->wrapper = mbuf;
    pd->data = rte_pktmbuf_mtod(pd->wrapper, uint8_t*);
    pd->wrapper->pkt_len = packet_length;
    debug_mbuf(mbuf, "Result of encryption\n");

    rte_mempool_put_bulk(lcore_conf[lcore_id].crypto_pool, (void **)dequeued_ops[lcore_id], 1);

    reset_pd(pd);
    parse_packet(pd, 0, 0);
}

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
            n = rte_ring_dequeue_burst(lcdata->conf->async_queue, (void**)async_ops[lcore_id], CRYPTO_BURST_SIZE, NULL);
            if(n > 0)
            {
                if (rte_crypto_op_bulk_alloc(lcdata->conf->crypto_pool, RTE_CRYPTO_OP_TYPE_SYMMETRIC, enqueued_ops[lcore_id], n) == 0)
                    rte_exit(EXIT_FAILURE, "Not enough crypto operations available\n");
                for(i = 0; i < n; i++)
                    async_op_to_crypto_op(async_ops[lcore_id][i], enqueued_ops[lcore_id][i]);
                rte_mempool_put_bulk(async_pool, (void**)async_ops[lcore_id], n);
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