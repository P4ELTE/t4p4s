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

// -----------------------------------------------------------------------------
// GLOBALS

struct rte_mempool *context_pool;
struct rte_mempool *async_pool;
struct rte_ring    *context_buffer;

// -----------------------------------------------------------------------------
// INTERFACE

void async_init_storage();
void async_handle_packet(struct lcore_data* lcdata, packet_descriptor_t* pd, unsigned pkt_idx, uint32_t port_id, void (*handler_function)(void));
void main_loop_async(struct lcore_data* lcdata, packet_descriptor_t* pd);
void do_async_op(packet_descriptor_t* pd, enum async_op_type op);

// -----------------------------------------------------------------------------
// DEBUG

#define DBG_CONTEXT_SWAP_TO_MAIN        debug(T4LIT(Swapping to main context...,warning) "\n");
#define DBG_CONTEXT_SWAP_TO_PACKET(ctx) debug(T4LIT(Swapping to packet context (%p)...,warning) "\n", ctx);
static void debug_mbuf(struct rte_mbuf *mbuf, char* message)
{
    dbg_bytes(rte_pktmbuf_mtod(mbuf, uint8_t*), rte_pktmbuf_pkt_len(mbuf),
      "\n--------------------------------\n" T4LIT(%s, port) " (" T4LIT(%d) " bytes): ", message, rte_pktmbuf_pkt_len(mbuf));
}

// -----------------------------------------------------------------------------
// EXTERNS

extern struct lcore_conf   lcore_conf[RTE_MAX_LCORE];

// defined in dataplane.c
void init_headers(packet_descriptor_t* pd, lookup_table_t** tables);
void reset_headers(packet_descriptor_t* pd, lookup_table_t** tables);
void parse_packet(packet_descriptor_t* pd, lookup_table_t** tables, parser_state_t* pstate);
void emit_packet(packet_descriptor_t* pd, lookup_table_t** tables, parser_state_t* pstate);
void control_DeparserImpl(packet_descriptor_t* pd, lookup_table_t** tables, parser_state_t* pstate);

// -----------------------------------------------------------------------------
// SERIALIZATION AND DESERIALIZATION

static void reset_pd(packet_descriptor_t *pd)
{
    pd->dropped=0;
    pd->parsed_length = 0;
    pd->payload_length = rte_pktmbuf_pkt_len(pd->wrapper) - pd->parsed_length;
    pd->emit_hdrinst_count = 0;
    pd->is_emit_reordering = false;
}

static void resume_packet_handling(struct rte_mbuf *mbuf, struct lcore_data* lcdata, packet_descriptor_t *pd)
{
    debug_mbuf(mbuf, "after async");

    // Extracting extra content from the mbuf

    int packet_length = *(rte_pktmbuf_mtod(mbuf, int*));
    rte_pktmbuf_adj(mbuf, sizeof(int));

    void* context = *(rte_pktmbuf_mtod(mbuf, void**));
    rte_pktmbuf_adj(mbuf, sizeof(void*));

    // Resetting the pd

    init_headers(pd, 0);
    reset_headers(pd, 0);
    reset_pd(pd);
    pd->wrapper = mbuf;
    pd->data = rte_pktmbuf_mtod(pd->wrapper, uint8_t*);

    pd->wrapper->pkt_len = packet_length;
    pd->context = context;

    DBG_CONTEXT_SWAP_TO_PACKET(context)
    swapcontext(&lcdata->conf->main_loop_context, context);
    debug("Swapped back to main context.\n");
}

static void enqueue_packet_for_async(packet_descriptor_t* pd, enum async_op_type op_type, void* context)
{
    struct async_op *op;
    rte_mempool_get(async_pool, (void**)&op);
    op->op = op_type;
    op->data = pd->wrapper;

    int packet_length = op->data->pkt_len;
    int extra_length = 0;
    debug_mbuf(op->data, "enqueueing for async");

    // Adding extra content to the mbuf

    rte_pktmbuf_prepend(op->data, sizeof(void*));
    *(rte_pktmbuf_mtod(op->data, void**)) = context;
    extra_length += sizeof(void*);

    rte_pktmbuf_prepend(op->data, sizeof(int));
    *(rte_pktmbuf_mtod(op->data, int*)) = packet_length;
    extra_length += sizeof(int);

    rte_ring_enqueue(lcore_conf[rte_lcore_id()].async_queue, op);

    debug_mbuf(op->data, "enqueued for async");
}

// -----------------------------------------------------------------------------
// CALLBACKS

void async_init_storage()
{
    context_pool = rte_mempool_create("context_pool", (unsigned)1023, sizeof(ucontext_t) + CONTEXT_STACKSIZE, MEMPOOL_CACHE_SIZE, 0, NULL, NULL, NULL, NULL, 0, 0);
    if (context_pool == NULL) rte_exit(EXIT_FAILURE, "Cannot create context pool\n");
    async_pool = rte_mempool_create("async_pool", (unsigned)1023, sizeof(struct async_op), MEMPOOL_CACHE_SIZE, 0, NULL, NULL, NULL, NULL, 0, 0);
    if (async_pool == NULL) rte_exit(EXIT_FAILURE, "Cannot create async op pool\n");
    context_buffer = rte_ring_create("context_ring", (unsigned)32*1024, SOCKET_ID_ANY, 0 /*RING_F_SP_ENQ | RING_F_SC_DEQ */);
}

void async_handle_packet(struct lcore_data* lcdata, packet_descriptor_t* pd, unsigned pkt_idx, uint32_t port_id, void (*handler_function)(void))
{
    ucontext_t *context;
    rte_mempool_get(context_pool, (void**)&context);
    context->uc_stack.ss_sp = (ucontext_t*)context + 1; // the stack is supposed to be placed right after the context description
    context->uc_stack.ss_size = CONTEXT_STACKSIZE;
    context->uc_stack.ss_flags = 0;
    pd->context = context;
    debug("Packet being handled, context reference is %p\n", context);

    getcontext(context);
    context->uc_link = &lcdata->conf->main_loop_context;
    makecontext(context, handler_function, 4, lcdata, pd, port_id);
    DBG_CONTEXT_SWAP_TO_PACKET(context)
    swapcontext(&lcdata->conf->main_loop_context, context);
    debug("Swapped back to main context.\n");
}

void do_async_op(packet_descriptor_t* pd, enum async_op_type op)
{
    if(pd->context == NULL) return;

    void* context = pd->context;

    // saving standard metadata into context
    int metadata_length = pd->headers[header_instance_standard_metadata].length;
    uint8_t standard_metadata[metadata_length];
    memcpy(standard_metadata,
           pd->headers[header_instance_standard_metadata].pointer,
           metadata_length);

    // deparse
    control_DeparserImpl(pd, 0, 0);
    emit_packet(pd, 0, 0);

    // enqueue mbuf to async operation buffer
    enqueue_packet_for_async(pd, op, context);

    // suspend processing of packet and go back to the main context
    DBG_CONTEXT_SWAP_TO_MAIN
    swapcontext(context, &lcore_conf[rte_lcore_id()].main_loop_context);
    debug("Swapped back to packet context %p.\n", context);

    // parse
    reset_pd(pd);
    parse_packet(pd, 0, 0);

    // restoring standard metadata from context
    memcpy(pd->headers[header_instance_standard_metadata].pointer,
           standard_metadata,
           metadata_length);
}

ucontext_t* cs[RTE_MAX_LCORE][CRYPTO_BURST_SIZE];
struct async_op *async_ops[RTE_MAX_LCORE][CRYPTO_BURST_SIZE];

void main_loop_async(struct lcore_data* lcdata, packet_descriptor_t *pd)
{
    unsigned lcore_id = rte_lcore_id();
    unsigned n, i;

    if(rte_ring_count(context_buffer) > CRYPTO_BURST_SIZE)
    {
        n = rte_ring_dequeue_burst(context_buffer, (void**)cs[lcore_id], CRYPTO_BURST_SIZE, NULL);
        for(i = 0; i < n; i++)
            debug(T4LIT(Packet context %p is being freed up.,warning) "\n", cs[lcore_id][i]);
        rte_mempool_put_bulk(context_pool, (void**)cs[lcore_id], n);
    }

    if(rte_ring_count(lcdata->conf->async_queue) >= CRYPTO_BURST_SIZE)
    {
        n = rte_ring_dequeue_burst(lcdata->conf->async_queue, (void**)async_ops[lcore_id], CRYPTO_BURST_SIZE, NULL);
        if(n > 0)
        {
            rte_mempool_put_bulk(async_pool, (void**)async_ops[lcore_id], n);
        }
    }
}

