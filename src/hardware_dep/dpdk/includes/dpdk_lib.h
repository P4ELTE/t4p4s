// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#pragma once

//=============================================================================

#include <rte_atomic.h>
#include <rte_byteorder.h>
#include <rte_common.h>
#include <rte_cycles.h>
#include <rte_debug.h>
#include <rte_eal.h>
#include <rte_interrupts.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_memcpy.h>
#include <rte_memory.h>
#include <rte_mempool.h>
#include <rte_memzone.h>
#include <rte_pci.h>
#include <rte_per_lcore.h>
#include <rte_prefetch.h>
#include <rte_random.h>
#include <rte_ring.h>
#include <rte_string_fns.h>
#include <rte_vect.h>

#include <getopt.h>

//=============================================================================

#include "backend.h"
#include "dpdk_tables.h"
#include "ctrl_plane_backend.h"

//=============================================================================
// Backend-specific aliases

#include "aliases.h"
#include "stateful_memory.h"

#define MAX_ETHPORTS RTE_MAX_ETHPORTS

//=============================================================================

#include "dpdk_lib_conf.h"

//=============================================================================

uint8_t topbits_1(uint8_t data, int bits);
uint16_t topbits_2(uint16_t data, int bits);
uint32_t topbits_4(uint32_t data, int bits);

uint8_t net2t4p4s_1(uint8_t data);
uint16_t net2t4p4s_2(uint16_t data);
uint32_t net2t4p4s_4(uint32_t data);

uint8_t t4p4s2net_1(uint8_t data);
uint16_t t4p4s2net_2(uint16_t data);
uint32_t t4p4s2net_4(uint32_t data);

//=============================================================================
// Timings

#define TABCHANGE_SLEEP_MICROS 200

// in debug mode, we assume that the controllers react briskly
#ifdef T4P4S_DEBUG
#define DIGEST_SLEEP_MILLIS    200
#else
#define DIGEST_SLEEP_MILLIS    1000
#endif
