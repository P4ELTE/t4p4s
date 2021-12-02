// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "backend.h"

#include "util_packet.h"
#include "common.h"

#include "dpdk_smem_repr.h"

typedef enum_CounterType_t T4P4S_COUNTER_e;
typedef enum_MeterType_t T4P4S_METER_e;

#include "dpdk_model_v1model_funs.h"


#define T4P4S_MODEL v1model

#define INGRESS_META_FLD    FLD(all_metadatas,ingress_port)
#define EGRESS_META_FLD     FLD(all_metadatas,egress_port)
#define EGRESS_INIT_VALUE   0
// note: EGRESS_DROP_VALUE should not clash with T4P4S_BROADCAST_PORT
#define EGRESS_DROP_VALUE   200


void set_handle_packet_metadata(packet_descriptor_t* pd, uint32_t portid);

