// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Eotvos Lorand University, Budapest, Hungary

#if T4P4S_INIT_CRYPTO

#include "dpdkx_crypto.h"
#include <rte_errno.h>

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
// Implementation of P4 architecture externs

// defined in dpdkx_crypto_ops.c
extern void do_crypto_operation(crypto_task_type_e task_type, int offset, SHORT_STDPARAMS);

// defined in main_async.c
extern void do_crypto_task(packet_descriptor_t* pd, int offset, crypto_task_type_e op);

extern struct lcore_conf lcore_conf[RTE_MAX_LCORE];

void do_encryption_async(uint8_buffer_t offset, SHORT_STDPARAMS)
{
    #if ASYNC_MODE == ASYNC_MODE_CONTEXT
        if(pd->context != NULL){
            COUNTER_STEP(lcore_conf[rte_lcore_id()].doing_crypto_packet);
            do_crypto_task(pd, offset.buffer[0], CRYPTO_TASK_ENCRYPT);
        }else{
            debug(T4LIT(Cannot find the context. We cannot do an async operation!,error) "\n");
            COUNTER_STEP(lcore_conf[rte_lcore_id()].fwd_packet);
        }
    #elif ASYNC_MODE == ASYNC_MODE_PD
        if(pd->context != NULL) {
            if(pd->program_restore_phase == 0){
                COUNTER_STEP(lcore_conf[rte_lcore_id()].doing_crypto_packet);
                do_crypto_task(pd, offset.buffer[0], CRYPTO_TASK_ENCRYPT);
            }
        }else{
            COUNTER_STEP(lcore_conf[rte_lcore_id()].fwd_packet);
        }
    #elif ASYNC_MODE == ASYNC_MODE_SKIP
        COUNTER_STEP(lcore_conf[rte_lcore_id()].fwd_packet);
    #elif ASYNC_MODE == ASYNC_MODE_OFF
        do_crypto_task(pd, offset.buffer[0], CRYPTO_TASK_ENCRYPT);
    #else
        #error Not Supported Async mode
    #endif
}

void do_decryption_async(uint8_buffer_t offset, SHORT_STDPARAMS)
{
    #if ASYNC_MODE == ASYNC_MODE_CONTEXT
        if(pd->context != NULL) {
            do_crypto_task(pd, offset.buffer[0], CRYPTO_TASK_DECRYPT);
        }else{
            debug(T4LIT(Cannot find the context. We cannot do an async operation!,error) "\n");
        }
    #elif ASYNC_MODE == ASYNC_MODE_PD
        if(pd->context != NULL) {
            if(pd->program_restore_phase == 1){
                do_crypto_task(pd, offset.buffer[0], CRYPTO_TASK_DECRYPT);
            }
        }
    #elif ASYNC_MODE == ASYNC_MODE_SKIP
        ;
    #elif ASYNC_MODE == ASYNC_MODE_OFF
            do_crypto_task(pd, offset.buffer[0], CRYPTO_TASK_DECRYPT);
    #else
        #error Not Supported Async mode
    #endif
}

void EXTERNIMPL1(md5_hmac,u8s)(uint8_buffer_t offset, SHORT_STDPARAMS)
{
    do_crypto_operation(CRYPTO_TASK_MD5_HMAC, offset.buffer[0], SHORT_STDPARAMS_IN);
}

void EXTERNIMPL1(encrypt,u8s)(uint8_buffer_t offset, SHORT_STDPARAMS)
{
    do_crypto_operation(CRYPTO_TASK_ENCRYPT, offset.buffer[0], SHORT_STDPARAMS_IN);
}

void EXTERNIMPL1(decrypt,u8s)(uint8_buffer_t offset, SHORT_STDPARAMS)
{
    do_crypto_operation(CRYPTO_TASK_DECRYPT, offset.buffer[0], SHORT_STDPARAMS_IN);
}

void EXTERNIMPL1(async_encrypt, u8s)(uint8_buffer_t offset, SHORT_STDPARAMS)
{
    do_encryption_async(offset, SHORT_STDPARAMS_IN);
}
void EXTERNIMPL1(async_decrypt, u8s)(uint8_buffer_t offset, SHORT_STDPARAMS)
{
    do_decryption_async(offset, SHORT_STDPARAMS_IN);
}

#endif
