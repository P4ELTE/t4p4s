// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include <rte_cryptodev.h>

#include "dpdk_lib.h"
#include "dpdk_lib_conf_async.h"

#define MAX_SESSIONS         2048
#define NUM_MBUFS            2048
#define POOL_CACHE_SIZE      128
#define AES_CBC_IV_LENGTH    16
#define AES_CBC_KEY_LENGTH   16
#define IV_OFFSET            (sizeof(struct rte_crypto_op) + \
                             sizeof(struct rte_crypto_sym_op))

#define DEFAULT_XFORM \
        (struct rte_crypto_sym_xform) { \
            .next = NULL, \
            .type = RTE_CRYPTO_SYM_XFORM_CIPHER, \
            .cipher = { \
                .algo = RTE_CRYPTO_CIPHER_AES_CBC, \
                .key = { \
                    .length = AES_CBC_KEY_LENGTH \
                }, \
                .iv = { \
                    .offset = IV_OFFSET, \
                    .length = AES_CBC_IV_LENGTH \
                } \
            } \
        }


typedef enum {
    CRYPTO_TASK_ENCRYPT,
    CRYPTO_TASK_DECRYPT,
    CRYPTO_TASK_MD5_HMAC
} crypto_task_type_e;

typedef struct {
    crypto_task_type_e op;
    struct rte_mbuf* data;
    int offset;
    int padding_length;
    int plain_length_to_encrypt;
    int original_plain_length;
} crypto_task_s;
extern struct rte_mempool *crypto_task_pool;
#define MD5_DIGEST_LEN	16

// -----------------------------------------------------------------------------
// Interface

#define CRYPTO_DEVICE_AVAILABLE (cdev_id != -1)

extern int cdev_id;
extern struct rte_cryptodev_sym_session *session_encrypt;
extern struct rte_cryptodev_sym_session *session_decrypt;
extern struct rte_mempool *crypto_pool;

void init_crypto_devices();
void crypto_task_to_crypto_op(crypto_task_s *crypto_task, struct rte_crypto_op *crypto_op);

void do_encryption_async_impl(SHORT_STDPARAMS);
void do_decryption_async_impl(SHORT_STDPARAMS);
