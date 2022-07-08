// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include <stdbool.h>
#include <rte_mempool.h>
#include <rte_ring.h>
#include <rte_version.h>


#define ASYNC_MODE_OFF 0
#define ASYNC_MODE_CONTEXT 1
#define ASYNC_MODE_PD 2
#define ASYNC_MODE_SKIP 3

#ifndef ASYNC_MODE
	#define ASYNC_MODE ASYNC_MODE_OFF
#endif

//#//define START_CRYPTO_NODE
#define CRYPTO_NODE_MODE_OPENSSL 1
#define CRYPTO_NODE_MODE_FAKE 2
#ifndef CRYPTO_NODE_MODE
    #define CRYPTO_NODE_MODE CRYPTO_NODE_MODE_OPENSSL
#endif


#ifndef FAKE_CRYPTO_SLEEP_MULTIPLIER
	#define FAKE_CRYPTO_SLEEP_MULTIPLIER 5000
#endif


#define increase_with_rotation(value,rotation_length) (value = (((value) >= (rotation_length-1)) ? ((value + 1) % (rotation_length)) : ((value) + 1)))

#if ASYNC_MODE == ASYNC_MODE_CONTEXT || ASYNC_MODE == ASYNC_MODE_PD
	#ifdef DEBUG__CRYPTO_EVERY_N
		#define PACKET_REQUIRES_ASYNC(lcdata,pd) (increase_with_rotation(lcdata->conf->crypto_every_n_counter, DEBUG__CRYPTO_EVERY_N)) == 0
    #else
		#define PACKET_REQUIRES_ASYNC(lcdata,pd) true
    #endif

    #ifndef CRYPTO_BURST_SIZE
	    #define CRYPTO_BURST_SIZE 64
    #endif
#else
	#define PACKET_REQUIRES_ASYNC(lcdata,pd) false
    #ifndef CRYPTO_BURST_SIZE
    	#define CRYPTO_BURST_SIZE 1
    #endif
#endif

#ifndef CRYPTO_CONTEXT_POOL_SIZE
	#define CRYPTO_CONTEXT_POOL_SIZE 1
#endif
#ifndef FAKE_CRYPTO_COMMAND_RING_SIZE
	#define FAKE_CRYPTO_COMMAND_RING_SIZE 32
#endif
#ifndef CONTEXT_FREE_COMMAND_RING_SIZE
    #define CONTEXT_FREE_COMMAND_RING_SIZE 32
#endif

#define DEBUG__COUNT_CONTEXT_MISSING_CAUSED_PACKET_DROP

extern struct rte_mempool *context_pool;
extern struct rte_ring *context_free_command_ring;
