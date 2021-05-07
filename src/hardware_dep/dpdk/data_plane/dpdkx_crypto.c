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

#if T4P4S_INIT_CRYPTO

#include "dpdkx_crypto.h"
#include <time.h>
#include <stdlib.h>
#include <rte_dev.h>
#include <rte_bus_vdev.h>
#ifdef RTE_LIBRTE_PMD_CRYPTO_SCHEDULER
#include <rte_cryptodev_scheduler.h>
#include <dataplane.h>

#endif

// -----------------------------------------------------------------------------
// Globals: memory pools, device and sessions

struct rte_mempool *session_pool, *session_priv_pool;
struct rte_mempool *crypto_pool;

int cdev_id;

struct rte_cryptodev_sym_session *session_encrypt;
struct rte_cryptodev_sym_session *session_decrypt;

uint8_t iv[16];

// -----------------------------------------------------------------------------
// Device initialisation and setup

static void setup_session(struct rte_cryptodev_sym_session **session, struct rte_mempool *session_pool)
{
    *session = rte_cryptodev_sym_session_create(session_pool);
    if (*session == NULL) rte_exit(EXIT_FAILURE, "Session could not be created\n");
}

static void init_session(int cdev_id, struct rte_cryptodev_sym_session *session, struct rte_crypto_sym_xform *xform, struct rte_mempool *session_priv_pool)
{
    if (rte_cryptodev_sym_session_init(cdev_id, session, xform, session_priv_pool) < 0)
        rte_exit(EXIT_FAILURE, "Session could not be initialized for the crypto device\n");
}

#define CRYPTO_MODE_OPENSSL 1
#define CRYPTO_MODE_NULL 2

#define CRYPTO_MODE CRYPTO_MODE_OPENSSL

static void setup_sessions()
{
    uint8_t cipher_key[16] = {0};

    struct rte_crypto_sym_xform cipher_xform_encrypt = DEFAULT_XFORM;
    #if CRYPTO_MODE == CRYPTO_MODE_OPENSSL
        cipher_xform_encrypt.cipher.op = RTE_CRYPTO_CIPHER_OP_ENCRYPT;
        cipher_xform_encrypt.cipher.key.data = cipher_key;
    #elif CRYPTO_MODE == CRYPTO_MODE_NULL
        cipher_xform_encrypt.cipher.algo = RTE_CRYPTO_CIPHER_NULL;
    #endif

    struct rte_crypto_sym_xform cipher_xform_decrypt = DEFAULT_XFORM;
    #if CRYPTO_MODE == CRYPTO_MODE_OPENSSL
        cipher_xform_decrypt.cipher.op = RTE_CRYPTO_CIPHER_OP_DECRYPT;
        cipher_xform_decrypt.cipher.key.data = cipher_key;
    #elif CRYPTO_MODE == CRYPTO_MODE_NULL
        cipher_xform_decrypt.cipher.algo = RTE_CRYPTO_CIPHER_NULL;
    #endif

    setup_session(&session_encrypt, session_pool);
    setup_session(&session_decrypt, session_pool);
    init_session(cdev_id, session_encrypt, &cipher_xform_encrypt, session_priv_pool);
    init_session(cdev_id, session_decrypt, &cipher_xform_decrypt, session_priv_pool);
}

static void crypto_init_storage(unsigned int session_size, uint8_t socket_id)
{
    session_pool = rte_cryptodev_sym_session_pool_create("session_pool", MAX_SESSIONS, session_size, POOL_CACHE_SIZE, 0, socket_id);
    session_priv_pool = session_pool;

    unsigned int crypto_op_private_data = AES_CBC_IV_LENGTH;
    crypto_pool = rte_crypto_op_pool_create("crypto_pool", RTE_CRYPTO_OP_TYPE_SYMMETRIC, 16*1024, POOL_CACHE_SIZE, crypto_op_private_data, socket_id);
    if (crypto_pool == NULL) rte_exit(EXIT_FAILURE, "Cannot create crypto op pool\n");
}

static int setup_device(const char *crypto_name, uint8_t socket_id)
{
    int ret;
    char args[128];
    snprintf(args, sizeof(args), "socket_id=%d", socket_id);
    ret = rte_vdev_init(crypto_name, args);
    if (ret != 0)
        debug("Cannot create crypto device " T4LIT(%s,error) "\n", crypto_name);
    return rte_cryptodev_get_dev_id(crypto_name);
}

static void configure_device(int cdev_id, struct rte_cryptodev_config *conf, struct rte_cryptodev_qp_conf *qp_conf, uint8_t socket_id)
{
    if (rte_cryptodev_configure(cdev_id, conf) < 0)
        rte_exit(EXIT_FAILURE, "Failed to configure cryptodev %u", cdev_id);
    int i;
    for(i = 0; i < conf-> nb_queue_pairs; i++)
        if (rte_cryptodev_queue_pair_setup(cdev_id, i, qp_conf, socket_id) < 0)
            rte_exit(EXIT_FAILURE, "Failed to setup queue pair\n");
    if (rte_cryptodev_start(cdev_id) < 0)
        rte_exit(EXIT_FAILURE, "Failed to start device\n");
}

// -----------------------------------------------------------------------------
// Callbacks

void init_crypto_devices()
{
    unsigned int session_size;
    uint8_t socket_id = rte_socket_id();

    #if CRYPTO_MODE == CRYPTO_MODE_OPENSSL
        cdev_id = setup_device("crypto_openssl0", socket_id);
    #elif CRYPTO_MODE == CRYPTO_MODE_NULL
        cdev_id = setup_device("crypto_null", socket_id);
    #endif

    if(CRYPTO_DEVICE_AVAILABLE)
    {
        session_size = rte_cryptodev_sym_get_private_session_size(cdev_id);
        crypto_init_storage(session_size, socket_id);
        struct rte_cryptodev_config conf = {
            .nb_queue_pairs = 8,
            .socket_id = socket_id
        };
        struct rte_cryptodev_qp_conf qp_conf = {
            .nb_descriptors = 2048,
            .mp_session = session_pool,
            .mp_session_private = session_priv_pool
        };
        configure_device(cdev_id, &conf, &qp_conf, socket_id);
        setup_sessions();
        srand(time(NULL));
        for(int i = 0; i < 16; i++) iv[i] = rand();
    }
    else
    {
        debug(T4LIT(Failed to setup crypto devices. Crypto operations are not available.,warning) "\n");
    }
}

void async_op_to_crypto_op(struct async_op *async_op, struct rte_crypto_op *crypto_op)
{
    crypto_op->sym->m_src = async_op->data;
    crypto_op->sym->cipher.data.offset = async_op->offset;
    crypto_op->sym->cipher.data.length = rte_pktmbuf_pkt_len(crypto_op->sym->m_src) - async_op->offset;

    uint8_t *iv_ptr = rte_crypto_op_ctod_offset(crypto_op, uint8_t *, IV_OFFSET);
    memcpy(iv_ptr, iv, AES_CBC_IV_LENGTH);
    switch(async_op->op)
    {
    case ASYNC_OP_ENCRYPT:
        rte_crypto_op_attach_sym_session(crypto_op, session_encrypt);
        break;
    case ASYNC_OP_DECRYPT:
        rte_crypto_op_attach_sym_session(crypto_op, session_decrypt);
        break;
    }
}

// -----------------------------------------------------------------------------
// Implementation of P4 architecture externs

// defined in main_async.c
void do_async_op(packet_descriptor_t* pd, enum async_op_type op);
void do_encryption(SHORT_STDPARAMS);
void do_decryption(SHORT_STDPARAMS);

extern struct lcore_conf   lcore_conf[RTE_MAX_LCORE];

void do_encryption_async(SHORT_STDPARAMS)
{
    #if ASYNC_MODE == ASYNC_MODE_CONTEXT
        if(pd->context != NULL){
            COUNTER_STEP(lcore_conf[rte_lcore_id()].doing_crypto_packet);
            do_async_op(pd, ASYNC_OP_ENCRYPT);
        }else{
            debug(T4LIT(Cannot find the context. We cannot do an async operation!,error) "\n");
            COUNTER_STEP(lcore_conf[rte_lcore_id()].fwd_packet);
        }
    #elif ASYNC_MODE == ASYNC_MODE_PD
        if(pd->context != NULL) {
            //debug("-----------------------------------------------Encrypt command, Program State: %d\n",pd->program_state)
            if(pd->program_state == 0){
                COUNTER_STEP(lcore_conf[rte_lcore_id()].doing_crypto_packet);
                do_async_op(pd, ASYNC_OP_ENCRYPT);
            }
        }else{
            COUNTER_STEP(lcore_conf[rte_lcore_id()].fwd_packet);
        }
    #elif ASYNC_MODE == ASYNC_MODE_SKIP
        COUNTER_STEP(lcore_conf[rte_lcore_id()].fwd_packet);
    #elif ASYNC_MODE == ASYNC_MODE_OFF
        do_encryption(SHORT_STDPARAMS_IN);
    #else
        #error Not Supported Async mode
    #endif
}

void do_decryption_async(SHORT_STDPARAMS)
{
    #if ASYNC_MODE == ASYNC_MODE_CONTEXT
        if(pd->context != NULL) {
            do_async_op(pd, ASYNC_OP_DECRYPT);
        }else{
            debug(T4LIT(Cannot find the context. We cannot do an async operation!,error) "\n");
        }
    #elif ASYNC_MODE == ASYNC_MODE_PD
        if(pd->context != NULL) {
            //debug("-----------------------------------------------DECRYPT command, Program State: %d\n",pd->program_state)
            if(pd->program_state == 1){
                do_async_op(pd, ASYNC_OP_DECRYPT);
            }
        }
    #elif ASYNC_MODE == ASYNC_MODE_SKIP
        ;
    #elif ASYNC_MODE == ASYNC_MODE_OFF
        do_decryption(SHORT_STDPARAMS_IN);
    #else
        #error Not Supported Async mode
    #endif
}

// defined in main_async.c
void do_blocking_sync_op(packet_descriptor_t* pd, enum async_op_type op);
void do_encryption(SHORT_STDPARAMS)
{
    #ifdef DEBUG__CRYPTO_EVERY_N
        if(lcore_conf[rte_lcore_id()].crypto_every_n_counter == 0){
            COUNTER_STEP(lcore_conf[rte_lcore_id()].doing_crypto_packet);
            COUNTER_STEP(lcore_conf[rte_lcore_id()].sent_to_crypto_packet);
            do_blocking_sync_op(pd, ASYNC_OP_ENCRYPT);
        }else{
            COUNTER_STEP(lcore_conf[rte_lcore_id()].fwd_packet);
        }
    #else
        do_blocking_sync_op(pd, ASYNC_OP_ENCRYPT);
    #endif
}

void do_decryption(SHORT_STDPARAMS)
{
    #ifdef DEBUG__CRYPTO_EVERY_N
        if(lcore_conf[rte_lcore_id()].crypto_every_n_counter == 0) {
            do_blocking_sync_op(pd, ASYNC_OP_DECRYPT);
        }
        increase_with_rotation(lcore_conf[rte_lcore_id()].crypto_every_n_counter, DEBUG__CRYPTO_EVERY_N);
    #else
        do_blocking_sync_op(pd, ASYNC_OP_DECRYPT);
    #endif
}



void do_ipsec_encapsulation(SHORT_STDPARAMS) {
    debug_mbuf(pd->wrapper,"do_ipsec_encapsulation, wrapper:");
    fprintf(stderr,"HEEEE:%d\n",pd->headers[1].length);
    dbg_bytes(pd->headers[1].pointer,pd->headers[1].length,"hello");

    int wrapper_length = rte_pktmbuf_pkt_len(pd->wrapper);
    int padding_length = (16 - wrapper_length% 16) % 16;
    int wrapper_and_payload_length = padding_length + wrapper_length;
    // 1 byte pad length, 1 byte next header, 8 byte esp, 8 byte iv, 12 byte hmac
    u_int8_t* new_payload = malloc(wrapper_and_payload_length + 1 + 1 + 8 + 8 + 12);
    memset(new_payload, 0, wrapper_and_payload_length + 1 + 1 + 8 + 8 + 12);

    // the output of the encryption will be after the IV and the simulated ESP
    u_int8_t* payload_to_encrypt = new_payload + 8 + 8;

    // copy original payload
    memcpy(payload_to_encrypt, rte_pktmbuf_mtod(pd->wrapper, uint8_t*), wrapper_length);
    //add padding
    memset(payload_to_encrypt+wrapper_length,0,padding_length);

    payload_to_encrypt[wrapper_and_payload_length] = (u_int8_t)padding_length;
    payload_to_encrypt[wrapper_and_payload_length + 1] = 4;

    dbg_bytes(new_payload + 8,wrapper_and_payload_length+2 + 8 + 8 + 12,"all buffer:");
    dbg_bytes(payload_to_encrypt,wrapper_and_payload_length+2,"To Encrypt:");
    // TODO: ENCRYPT
    u_int8_t* iv = malloc(8);
    memset(iv,0xbb,8);

    u_int32_t spi = 0xee;
    u_int32_t esp_counter = 0xcc;

    memcpy(new_payload,&spi,4);
    memcpy(new_payload + 4,&esp_counter,4);
    memcpy(new_payload + 8,iv,8);


    u_int8_t* hmac = malloc(12);
    memset(hmac,0xaa,12);
    int hmac_length;
    //TODO: hmac(hmac,&hmac_length,new_payload,8 + 8 + wrapper_and_payload_length + 2);
    memcpy(new_payload + 8 + 8 + wrapper_and_payload_length + 2, hmac, 12);

    dbg_bytes(new_payload,wrapper_and_payload_length+2 + 8 + 8 + 12,"all buffer:");
    dbg_bytes(new_payload + 8,wrapper_and_payload_length+2 + 8 + 12,"final");
}

#endif