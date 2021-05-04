// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include <rte_mbuf.h>
typedef struct rte_mbuf packet;

typedef uint8_t packet_data_t;

#include <rte_spinlock.h>
typedef rte_spinlock_t lock_t;

#define INVALID_TABLE_ENTRY false
#define VALID_TABLE_ENTRY   true

#define FLD_ATTR(hdr,fld) attr_field_instance_##hdr##_##fld
#define FLD(hdr,fld) field_instance_##hdr##_##fld
#define HDR(hdr) header_##hdr
#define STK(stk) stack_##stk
