// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_psa.h"

#include <rte_ip.h>
#include "actions.h"
#include "util_debug.h"

#include "dpdk_lib.h"
#include "stateful_memory.h"

#include "gen_model.h"


extern void do_counter_count(counter_t* counter, int index, uint32_t value);

void extern_counter_count(uint32_t counter_array_size, T4P4S_COUNTER_e ct, uint32_t index, counter_t* counter, SHORT_STDPARAMS) {
    do_counter_count(counter, index, ct == enum_PSA_CounterType_t_PACKETS ? 1 : packet_size(pd));
}

void extern_direct_counter_count(int counter_type, direct_counter_t* smem, SHORT_STDPARAMS) {
    debug("    : Executing extern_direct_counter_count\n");
}


void extern_meter_execute_meter__u32(uint32_t index, T4P4S_METER_e b, uint32_t c, uint8_t* d, meter_t* e, SHORT_STDPARAMS) {
    debug("    : Executing extern_meter_execute_meter__u32#" T4LIT(%d) "\n", index);
}

void extern_direct_meter_read__u8(T4P4S_METER_e b, uint32_t c, meter_t* e, SHORT_STDPARAMS) {
    debug("    : Executing extern_direct_meter_read__u8\n");
}


void extern_register_read(uint32_t index, uint32_t a, uint32_t b, register_t c, SHORT_STDPARAMS) {
    debug("    : Executing extern_register_read#" T4LIT(%d) "\n", index);
}

void extern_register_write(uint32_t index, uint32_t a, uint32_t b, register_t* c, SHORT_STDPARAMS) {
    debug("    : Executing extern_register_write#" T4LIT(%d) "\n", index);
}
