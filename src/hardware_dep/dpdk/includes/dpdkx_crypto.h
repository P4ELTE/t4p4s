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

#ifndef DPDKX_CRYPTO_H
#define DPDKX_CRYPTO_H

#include <rte_cryptodev.h>
#include "dpdk_lib.h"

#define MAX_SESSIONS         2048
#define NUM_MBUFS            2048
#define POOL_CACHE_SIZE      128
#define AES_CBC_IV_LENGTH    16
#define AES_CBC_KEY_LENGTH   16
#define IV_OFFSET            (sizeof(struct rte_crypto_op) + \
                             sizeof(struct rte_crypto_sym_op))

#define DEFAULT_XFORM \
        { \
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

// -----------------------------------------------------------------------------
// Interface

#define CRYPTO_DEVICE_AVAILABLE (cdev_id != -1)

extern int cdev_id;
extern struct rte_cryptodev_sym_session *session_encrypt;
extern struct rte_cryptodev_sym_session *session_decrypt;
extern struct rte_mempool *crypto_pool;

void init_crypto_devices();
void async_op_to_crypto_op(struct async_op *async_op, struct rte_crypto_op *crypto_op);

#endif

