// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Eotvos Lorand University, Budapest, Hungary

#if T4P4S_INIT_CRYPTO

#include "dpdkx_crypto.h"
#include "aliases.h"

#include <stdlib.h>
#include <rte_dev.h>
#include <error.h>
#include <util_debug.h>
#include <unistd.h>
#include <iso646.h>

extern struct lcore_conf lcore_conf[RTE_MAX_LCORE];

const char* crypto_task_type_names[] = {
    "ENCRYPT",
    "DECRYPT",
    "MD5_HMAC",
};

struct rte_mempool *session_pool, *session_priv_pool;

int cdev_id;

struct rte_cryptodev_sym_session *session_encrypt;
struct rte_cryptodev_sym_session *session_decrypt;
struct rte_cryptodev_sym_session *session_hmac;
uint8_t iv[16];


struct rte_mempool *crypto_task_pool;
crypto_task_s *crypto_tasks[RTE_MAX_LCORE][CRYPTO_BURST_SIZE];

struct rte_mempool *rte_crypto_op_pool;
struct rte_crypto_op* enqueued_rte_crypto_ops[RTE_MAX_LCORE][CRYPTO_BURST_SIZE];
struct rte_crypto_op* dequeued_rte_crypto_ops[RTE_MAX_LCORE][CRYPTO_BURST_SIZE];

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

void debug_crypto_task(crypto_task_s *op) {
    #ifdef T4P4S_DEBUG
        dbg_mbuf(op->data, " " T4LIT(<<<<,outgoing) " Sending packet to " T4LIT(crypto device,outgoing) " for " T4LIT(%s,extern) " operation", crypto_task_type_names[op->type])

        uint8_t* buf = rte_pktmbuf_mtod(op->data, uint8_t*);
        dbg_bytes(buf, sizeof(uint32_t), "   :: Size info (" T4LIT(%luB) "): ", sizeof(uint32_t));
        dbg_bytes(buf + op->offset, op->plain_length_to_encrypt, "   :: Data (" T4LIT(%dB) ")    : ", op->plain_length_to_encrypt);
        debug("   :: plain_length_to_encrypt: %d\n",op->plain_length_to_encrypt);
        debug("   :: offset: %d\n",op->offset);
        debug("   :: padding_length: %d\n",op->padding_length);
    #endif
}
// -----------------------------------------------------------------------------
// Crypto operations

int prepare_symmetric_ecnryption_op(crypto_task_s *task, struct rte_crypto_op *crypto_op) {
    crypto_op->sym->m_src = task->data;
    crypto_op->sym->cipher.data.offset = task->offset;
    crypto_op->sym->cipher.data.length = task->plain_length_to_encrypt + task->padding_length;

    uint8_t *iv_ptr = rte_crypto_op_ctod_offset(crypto_op, uint8_t *, IV_OFFSET);
    memcpy(iv_ptr, iv, AES_CBC_IV_LENGTH);

    switch(task->type)
    {
        case CRYPTO_TASK_ENCRYPT:
            rte_crypto_op_attach_sym_session(crypto_op, session_encrypt);
            break;
        case CRYPTO_TASK_DECRYPT:
            rte_crypto_op_attach_sym_session(crypto_op, session_decrypt);
            break;
        default:
            return 1;
    }
    return 0;
}

void process_symmetric_encryption_result(crypto_task_type_e task_type, packet_descriptor_t *pd) {
    unsigned int lcore_id = rte_lcore_id();

    pd->data = rte_pktmbuf_mtod(pd->wrapper, uint8_t*);
    dbg_bytes(pd->data, pd->wrapper->pkt_len,
              " " T4LIT( >> >> , incoming) " Result of " T4LIT(%s, extern) " crypto operation (" T4LIT(%dB) "): ",
              crypto_task_type_names[task_type], rte_pktmbuf_pkt_len(pd->wrapper));
}

void prepare_hmac_op(crypto_task_s *task, struct rte_crypto_op *crypto_op) {
    crypto_op->sym->auth.digest.data = (uint8_t *)rte_pktmbuf_append(task->data, MD5_DIGEST_LEN);
    crypto_op->sym->auth.data.offset = task->offset;
    crypto_op->sym->auth.data.length = task->plain_length_to_encrypt;

    rte_crypto_op_attach_sym_session(crypto_op, session_hmac);
    crypto_op->sym->m_src = task->data;
}


void process_hmac_result(crypto_task_type_e task_type, packet_descriptor_t *pd,
                         crypto_task_s *crypto_task) {
    unsigned int lcore_id = rte_lcore_id();

    uint8_t* wrapper_pointer = rte_pktmbuf_mtod(pd->wrapper, uint8_t*);
    uint32_t target_relative = crypto_task->offset +
                               crypto_task->plain_length_to_encrypt;
    uint8_t* target = wrapper_pointer + target_relative;
    if (dequeued_rte_crypto_ops[lcore_id][0]->sym->m_dst) {
        uint8_t *auth_tag = rte_pktmbuf_mtod_offset(dequeued_rte_crypto_ops[lcore_id][0]->sym->m_dst, uint8_t *,
                                                    crypto_task->padding_length);

        memmove(target,auth_tag,MD5_DIGEST_LEN);
        rte_pktmbuf_trim(pd->wrapper,crypto_task->padding_length);
    }else {
        uint8_t from_relative = crypto_task->offset +
                                crypto_task->plain_length_to_encrypt +
                                crypto_task->padding_length;
        uint8_t*  from_position = wrapper_pointer + from_relative;

        memmove(target,from_position,MD5_DIGEST_LEN);
        rte_pktmbuf_trim(pd->wrapper,crypto_task->padding_length);
    }
    dbg_bytes(rte_pktmbuf_mtod(pd->wrapper, uint8_t*), rte_pktmbuf_pkt_len(pd->wrapper),
              " " T4LIT( >> >> , incoming) " Result of " T4LIT(%s, extern) " crypto operation (" T4LIT(%dB) "): ",
              crypto_task_type_names[task_type], rte_pktmbuf_pkt_len(pd->wrapper));
}

int process_crypto_op_result(crypto_task_type_e task_type, packet_descriptor_t *pd,
                             crypto_task_s *crypto_task) {
    unsigned int lcore_id = rte_lcore_id();
    int ret = 0;
    switch(task_type){
        case CRYPTO_TASK_MD5_HMAC:
            process_hmac_result(task_type, pd, crypto_task);
            break;

        case CRYPTO_TASK_ENCRYPT:
        case CRYPTO_TASK_DECRYPT:
            process_symmetric_encryption_result(task_type, pd);
            break;
        default:
            ret = 1;
            break;
    }
    return ret;
}

void crypto_task_to_rte_crypto_op(crypto_task_s *task, struct rte_crypto_op *crypto_op)
{
    if(task->type == CRYPTO_TASK_MD5_HMAC){
        prepare_hmac_op(task, crypto_op);
    }else {
        prepare_symmetric_ecnryption_op(task, crypto_op);
    }
}




// -----------------------------------------------------------------------------
// General functions

crypto_task_s* create_crypto_task(crypto_task_s **task_out, packet_descriptor_t* pd, crypto_task_type_e task_type, int offset, packet_descriptor_t* pd_copy, void* extraInformationForAsyncHandling){
    int ret = rte_mempool_get(crypto_task_pool, (void**)task_out);
    if(ret < 0){
        rte_exit(EXIT_FAILURE, "Mempool get failed!\n");
        //TODO: it should be a packet drop, not total fail
    }
    crypto_task_s *task = *task_out;
    task->type = task_type;
    #if ASYNC_MODE == ASYNC_MODE_PD
        task->data = pd_copy->wrapper;
    #else
        task->data = pd->wrapper;
    #endif

    task->offset = offset;

    task->original_plain_length = task->data->pkt_len;
    task->plain_length_to_encrypt = task->data->pkt_len - task->offset;

#if ASYNC_MODE == ASYNC_MODE_CONTEXT
    if(extraInformationForAsyncHandling != NULL){
        void* context = extraInformationForAsyncHandling;
        rte_pktmbuf_prepend(task->data, sizeof(void*));
        *(rte_pktmbuf_mtod(task->data, void**)) = context;
        task->offset += sizeof(void*);
    }
#elif ASYNC_MODE == ASYNC_MODE_PD
    rte_pktmbuf_prepend(task->data, sizeof(packet_descriptor_t*));
    *(rte_pktmbuf_mtod(task->data, packet_descriptor_t**)) = pd_copy;
    debug("Save pd_copy address: %d\n",pd_copy);

    task->offset += sizeof(void**);
#endif

    task->padding_length = (16 - task->plain_length_to_encrypt % 16) % 16;

    if(task->padding_length){
        void* padding_memory = rte_pktmbuf_append(task->data, task->padding_length);
        memset(padding_memory, 0, task->padding_length);
    }
    return task;
}

int enqueue_crypto_ops(uint16_t number_of_ops){
    unsigned int lcore_id = rte_lcore_id();
    #ifdef START_CRYPTO_NODE
        return rte_ring_enqueue_burst(lcore_conf[lcore_id].fake_crypto_rx, enqueued_rte_crypto_ops[lcore_id], number_of_ops, NULL);
    #else
        return rte_cryptodev_enqueue_burst(cdev_id, lcore_id, enqueued_rte_crypto_ops[lcore_id], number_of_ops);
    #endif
}

void dequeue_crypto_ops_blocking(uint16_t number_of_ops){
    unsigned int lcore_id = rte_lcore_id();
    #ifdef START_CRYPTO_NODE
        while(rte_ring_dequeue_burst(lcore_conf[lcore_id].fake_crypto_tx, (void**)dequeued_rte_crypto_ops[lcore_id], number_of_ops, NULL) == 0);
    #else
        while(rte_cryptodev_dequeue_burst(cdev_id, lcore_id, dequeued_rte_crypto_ops[lcore_id], number_of_ops) == 0);
    #endif
}




void execute_task_blocking(crypto_task_type_e task_type, packet_descriptor_t *pd, unsigned int lcore_id,
                           crypto_task_s* crypto_task) {
    debug_crypto_task(crypto_task);

    if (rte_crypto_op_bulk_alloc(rte_crypto_op_pool, RTE_CRYPTO_OP_TYPE_SYMMETRIC, &enqueued_rte_crypto_ops[lcore_id][0], 1) == 0){
        rte_exit(EXIT_FAILURE, "Not enough crypto operations available\n");
    }

    crypto_task_to_rte_crypto_op(crypto_task, enqueued_rte_crypto_ops[lcore_id][0]);
    rte_mempool_put_bulk(crypto_task_pool, (void**) &crypto_task, 1);

    if(enqueue_crypto_ops(1) == 0){
        debug(" " T4LIT(!!!!,error) " " T4LIT(Enqueue ops in blocking sync task_type failed... skipping encryption,error) "\n");
        return;
    }
    dequeue_crypto_ops_blocking(1);

    if(process_crypto_op_result(task_type, pd, crypto_task) > 0){
        debug(" " T4LIT(!!!!,error) " " T4LIT((*task_type) not found ... skipping,error) "\n");
    }

    rte_mempool_put_bulk(rte_crypto_op_pool, (void **)dequeued_rte_crypto_ops[lcore_id], 1);
    reset_pd(pd);
}

void do_crypto_operation(crypto_task_type_e task_type, int offset, SHORT_STDPARAMS){
    // deparse_packet(SHORT_STDPARAMS_IN);
    // pd->is_deparse_reordering = false;
    unsigned int lcore_id = rte_lcore_id();

    crypto_task_s* crypto_task = create_crypto_task(&crypto_tasks[lcore_id][0], pd, task_type, offset, 0, NULL);

    execute_task_blocking(task_type, pd, lcore_id, crypto_task);
}


#endif
