// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Eotvos Lorand University, Budapest, Hungary

#include "dpdkx_crypto.h"

#if T4P4S_INIT_CRYPTO

#include <time.h>
#include <stdlib.h>
#include <rte_dev.h>
#include <rte_bus_vdev.h>
#include <rte_errno.h>

#ifdef RTE_LIBRTE_PMD_CRYPTO_SCHEDULER
    #include <rte_cryptodev_scheduler.h>
    #include <dataplane.h>
#endif

// -----------------------------------------------------------------------------

extern struct rte_mempool *session_pool, *session_priv_pool;
extern struct rte_cryptodev_sym_session *session_hmac;

extern uint8_t iv[16];

// -----------------------------------------------------------------------------
// Device initialisation and setup

static void setup_session(struct rte_cryptodev_sym_session **session, struct rte_mempool *session_pool)
{
    *session = rte_cryptodev_sym_session_create(session_pool);
    if (*session == NULL){
        rte_exit(EXIT_FAILURE, "Session could not be created\n");
    }
}

static void init_session(int cdev_id, struct rte_cryptodev_sym_session *session, struct rte_crypto_sym_xform *xform, struct rte_mempool *session_priv_pool)
{
    if (rte_cryptodev_sym_session_init(cdev_id, session, xform, session_priv_pool) < 0){
        rte_exit(EXIT_FAILURE, "Session could not be initialized for the crypto device\n");
    }
}

#define CRYPTO_MODE_OPENSSL 1
#define CRYPTO_MODE_NULL 2

#define CRYPTO_MODE CRYPTO_MODE_OPENSSL



static void setup_sessions()
{
    uint8_t cipher_key[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

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


    struct rte_crypto_sym_xform cipher_xform_hmac;
    cipher_xform_hmac.type = RTE_CRYPTO_SYM_XFORM_AUTH;
	cipher_xform_hmac.next = NULL;
	cipher_xform_hmac.auth.op = RTE_CRYPTO_AUTH_OP_GENERATE;

	cipher_xform_hmac.auth.algo = RTE_CRYPTO_AUTH_MD5_HMAC;

	cipher_xform_hmac.auth.digest_length = MD5_DIGEST_LEN;
	cipher_xform_hmac.auth.key.length = 16;
	uint8_t key_data[16] = {0x68,0x65,0x6c,0x6c,0x6f,0x74,0x34,0x70,0x34,0x73,0x75,0x73,0x65,0x72,0x7a,0x7a};
	cipher_xform_hmac.auth.key.data = key_data;


    setup_session(&session_encrypt, session_pool);
    setup_session(&session_decrypt, session_pool);
    setup_session(&session_hmac, session_pool);
    init_session(cdev_id, session_encrypt, &cipher_xform_encrypt, session_priv_pool);
    init_session(cdev_id, session_decrypt, &cipher_xform_decrypt, session_priv_pool);
    init_session(cdev_id, session_hmac, &cipher_xform_hmac, session_priv_pool);
}

static void crypto_init_storage(unsigned int session_size, uint8_t socket_id)
{
    session_pool = rte_cryptodev_sym_session_pool_create("session_pool", MAX_SESSIONS, session_size, POOL_CACHE_SIZE, 0, socket_id);
    session_priv_pool = session_pool;

    unsigned int crypto_op_private_data = AES_CBC_IV_LENGTH;
    rte_crypto_op_pool = rte_crypto_op_pool_create("crypto_pool", RTE_CRYPTO_OP_TYPE_SYMMETRIC, 16 * 1024, POOL_CACHE_SIZE, crypto_op_private_data, socket_id);
    if (rte_crypto_op_pool == NULL) rte_exit(EXIT_FAILURE, "Cannot create crypto op pool\n");

    crypto_task_pool = rte_mempool_create("crypto_task_pool", (unsigned)16*1024-1, sizeof(crypto_task_s), MEMPOOL_CACHE_SIZE, 0, NULL, NULL, NULL, NULL, 0, 0);
    if (crypto_task_pool == NULL) {
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
        for(int i = 0; i < 16; i++) iv[i] = 0;//rand();
    }
    else
    {
        debug(" " T4LIT(!!!! Failed to setup crypto devices.,warning) " Crypto operations are not available.\n");
    }
}

#endif
