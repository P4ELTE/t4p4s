// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Eotvos Lorand University, Budapest, Hungary

#if T4P4S_INIT_CRYPTO

#include "dpdkx_crypto.h"
#include "aliases.h"

#include <time.h>
#include <stdlib.h>
#include <rte_dev.h>
#include <rte_bus_vdev.h>
#include <rte_errno.h>

#ifdef RTE_LIBRTE_PMD_CRYPTO_SCHEDULER
    #include <rte_cryptodev_scheduler.h>
    #include <dataplane.h>
#endif

extern void deparse_packet(SHORT_STDPARAMS);

const char*const crypto_task_type_names[] = {
    "ENCRYPT",
    "DECRYPT",
    "MD5_HMAC",
};

// -----------------------------------------------------------------------------
// Globals: memory pools, device and sessions

struct rte_mempool *session_pool, *session_priv_pool;
struct rte_mempool *crypto_pool;
struct rte_mempool *crypto_task_pool;

crypto_task_s *crypto_tasks[RTE_MAX_LCORE][CRYPTO_BURST_SIZE];
struct rte_crypto_op* enqueued_ops[RTE_MAX_LCORE][CRYPTO_BURST_SIZE];
struct rte_crypto_op* dequeued_ops[RTE_MAX_LCORE][CRYPTO_BURST_SIZE];


int cdev_id;

struct rte_cryptodev_sym_session *session_encrypt;
struct rte_cryptodev_sym_session *session_decrypt;
struct rte_cryptodev_sym_session *session_hmac;

uint8_t iv[16];
extern struct lcore_conf lcore_conf[RTE_MAX_LCORE];

// -----------------------------------------------------------------------------
//  Helper functions

void reset_pd(packet_descriptor_t *pd)
{
    pd->parsed_size = 0;
    if(pd->wrapper == 0){
        pd->payload_size = 0;
    }else{
        pd->payload_size = rte_pktmbuf_pkt_len(pd->wrapper) - pd->parsed_size;
    }
    pd->deparse_hdrinst_count = 0;
    pd->is_deparse_reordering = false;
}

void print_crypto_create_msg(crypto_task_s *op) {
    #ifdef T4P4S_DEBUG
        uint8_t* buf = rte_pktmbuf_mtod(op->data, uint8_t*);
        unsigned long total_length = op->padding_length + sizeof(uint32_t) + op->plain_length;
        dbg_bytes(buf, total_length, " " T4LIT(<<<<,outgoing) " Sending packet to " T4LIT(crypto device,outgoing) " for " T4LIT(%s,extern) " operation (" T4LIT(%luB) "): ", crypto_task_type_names[op->op], total_length);
        if (op->padding_length == 0) {
            dbg_bytes(buf, op->padding_length, "   :: Padding (" T4LIT(%dB) ")  : ", op->padding_length);
        }
        dbg_bytes(buf + op->padding_length, sizeof(uint32_t), "   :: Size info (" T4LIT(%luB) "): ", sizeof(uint32_t));
        dbg_bytes(buf + op->padding_length + 4, op->plain_length, "   :: Data (" T4LIT(%dB) ")    : ", op->plain_length);
    #endif
}

// -----------------------------------------------------------------------------
// Crypto operations

void create_crypto_op(crypto_task_s **op_out, packet_descriptor_t* pd, crypto_task_type_e op_type, int offset, void* extraInformationForAsyncHandling){
    int ret = rte_mempool_get(crypto_task_pool, (void**)op_out);
    if(ret < 0){
        rte_exit(EXIT_FAILURE, "Mempool get failed!\n");
        //TODO: it should be a packet drop, not total fail
    }
    crypto_task_s *op = *op_out;
    op->op = op_type;
    op->data = pd->wrapper;

    op->plain_length = op->data->pkt_len - offset;
    op->offset = offset;

    #if ASYNC_MODE == ASYNC_MODE_CONTEXT
        if(extraInformationForAsyncHandling != NULL){
            void* context = extraInformationForAsyncHandling;
            rte_pktmbuf_prepend(op->data, sizeof(void*));
            *(rte_pktmbuf_mtod(op->data, void**)) = context;
            op->offset += sizeof(void*);
        }
    #elif ASYNC_MODE == ASYNC_MODE_PD
        packet_descriptor_t *store_pd = extraInformationForAsyncHandling;
        *store_pd = *pd;

        rte_pktmbuf_prepend(op->data, sizeof(packet_descriptor_t*));
        *(rte_pktmbuf_mtod(op->data, packet_descriptor_t**)) = store_pd;
        op->offset += sizeof(void**);
    #endif

    rte_pktmbuf_prepend(op->data, sizeof(uint32_t));
    *(rte_pktmbuf_mtod(op->data, uint32_t*)) = op->plain_length;
    op->offset += sizeof(uint32_t);

    if(op->plain_length%16 != 0){
        op->padding_length = 16-op->plain_length%16;
        void* padding_memory = rte_pktmbuf_append(op->data, op->padding_length);
        memset(padding_memory,0,op->padding_length);
    }else{
        op->padding_length = 0;
    }

    print_crypto_create_msg(op);
}

void crypto_task_to_crypto_op(crypto_task_s *crypto_task, struct rte_crypto_op *crypto_op)
{
    if(crypto_task->op == CRYPTO_TASK_MD5_HMAC){
        crypto_op->sym->auth.digest.data = (uint8_t *)rte_pktmbuf_append(crypto_task->data, MD5_DIGEST_LEN);
        crypto_op->sym->auth.data.offset = crypto_task->offset;
        crypto_op->sym->auth.data.length = crypto_task->plain_length;

        rte_crypto_op_attach_sym_session(crypto_op, session_hmac);
        crypto_op->sym->m_src = crypto_task->data;
    }else{
        crypto_op->sym->m_src = crypto_task->data;
        crypto_op->sym->cipher.data.offset = crypto_task->offset;
        crypto_op->sym->cipher.data.length = rte_pktmbuf_pkt_len(crypto_op->sym->m_src) - crypto_task->offset;

        uint8_t *iv_ptr = rte_crypto_op_ctod_offset(crypto_op, uint8_t *, IV_OFFSET);
        memcpy(iv_ptr, iv, AES_CBC_IV_LENGTH);

        switch(crypto_task->op)
        {
        case CRYPTO_TASK_ENCRYPT:
            rte_crypto_op_attach_sym_session(crypto_op, session_encrypt);
            break;
        case CRYPTO_TASK_DECRYPT:
            rte_crypto_op_attach_sym_session(crypto_op, session_decrypt);
            break;
        case CRYPTO_TASK_MD5_HMAC:
            // TODO
            break;
        }
    }
}


void do_blocking_sync_op(crypto_task_type_e op, int offset, SHORT_STDPARAMS){
    // deparse_packet(SHORT_STDPARAMS_IN);
    // pd->is_deparse_reordering = false;

    unsigned int lcore_id = rte_lcore_id();

    create_crypto_op(crypto_tasks[lcore_id],pd,op,offset,NULL);
    if (rte_crypto_op_bulk_alloc(crypto_pool, RTE_CRYPTO_OP_TYPE_SYMMETRIC, enqueued_ops[lcore_id], 1) == 0){
        rte_exit(EXIT_FAILURE, "Not enough crypto operations available\n");
    }
    crypto_task_to_crypto_op(crypto_tasks[lcore_id][0], enqueued_ops[lcore_id][0]);
    rte_mempool_put_bulk(crypto_task_pool, (void **) crypto_tasks[lcore_id], 1);

    #ifdef START_CRYPTO_NODE
        if (rte_ring_enqueue_burst(lcore_conf[lcore_id].fake_crypto_rx, (void**)enqueued_ops[lcore_id], 1, NULL) <= 0){
            debug(" " T4LIT(!!!!,error) " " T4LIT(Enqueue ops in blocking sync op failed... skipping encryption,error) "\n");
            return;
        }
        while(rte_ring_dequeue_burst(lcore_conf[lcore_id].fake_crypto_tx, (void**)dequeued_ops[lcore_id], 1, NULL) == 0);
    #else
        if(rte_cryptodev_enqueue_burst(cdev_id, lcore_id,enqueued_ops[lcore_id], 1) <= 0){
            debug(" " T4LIT(!!!!,error) " " T4LIT(Enqueue ops in blocking sync op failed... skipping encryption,error) "\n");
            return;
        }
        while(rte_cryptodev_dequeue_burst(cdev_id, lcore_id, dequeued_ops[lcore_id], 1) == 0);
    #endif

    if(op == CRYPTO_TASK_MD5_HMAC){
        if (enqueued_ops[lcore_id][0]->sym->m_dst) {
            uint8_t *auth_tag = rte_pktmbuf_mtod_offset(enqueued_ops[lcore_id][0]->sym->m_dst,uint8_t *,
                                                       crypto_tasks[lcore_id][0]->padding_length);
            int target_position = crypto_tasks[lcore_id][0]->padding_length +
                               crypto_tasks[lcore_id][0]->plain_length +
                               crypto_tasks[lcore_id][0]->offset;
            uint8_t* target = rte_pktmbuf_mtod(pd->wrapper, uint8_t*) + target_position;
            memcpy(target,auth_tag,MD5_DIGEST_LEN);
        }
        rte_pktmbuf_adj(pd->wrapper, sizeof(int));
        dbg_bytes(pd->wrapper, rte_pktmbuf_pkt_len(pd->wrapper), " " T4LIT(>>>>,incoming) " Result of " T4LIT(%s,extern) " crypto operation (" T4LIT(%dB) "): ", crypto_task_type_names[op], rte_pktmbuf_pkt_len(pd->wrapper));
    }else{
        struct rte_mbuf *mbuf = dequeued_ops[lcore_id][0]->sym->m_src;
        int packet_size = *(rte_pktmbuf_mtod(mbuf, int*));

        rte_pktmbuf_adj(mbuf, sizeof(int));
        pd->wrapper = mbuf;
        pd->data = rte_pktmbuf_mtod(pd->wrapper, uint8_t*);
        pd->wrapper->pkt_len = packet_size;
        dbg_bytes(pd->wrapper, rte_pktmbuf_pkt_len(pd->wrapper), " " T4LIT(>>>>,incoming) " Result of " T4LIT(%s,extern) " crypto operation (" T4LIT(%dB) "): ", crypto_task_type_names[op], rte_pktmbuf_pkt_len(pd->wrapper));
    }

    rte_mempool_put_bulk(crypto_pool, (void **)dequeued_ops[lcore_id], 1);
    reset_pd(pd);
}

#endif
