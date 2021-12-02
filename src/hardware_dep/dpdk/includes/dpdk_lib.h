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
#include "dpdk_lib_byteorder.h"

//=============================================================================
// Timings

#define TABCHANGE_SLEEP_MICROS 200

// in debug mode, we assume that the controllers react briskly
#ifdef T4P4S_DEBUG
#define DIGEST_SLEEP_MILLIS    200
#else
#define DIGEST_SLEEP_MILLIS    1000
#endif

//=============================================================================
// Packet handling

typedef void (*packet_handler_t)(int port_id, unsigned queue_idx, unsigned pkt_idx, LCPARAMS);
typedef void (*packet_handler_noparams_t)();

//=============================================================================
// Externs

#include "dpdkx_gen_extern.h"

