// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_psa.h"

#include <rte_ip.h>
#include "actions.h"
#include "util_debug.h"

#include "dpdk_lib.h"
#include "stateful_memory.h"

#include "gen_model.h"


extern void do_counter_count(SMEMTYPE(counter)* counter, int index, uint32_t value);

void EXTERNCALL0(Counter,count)(uint32_t counter_array_size, T4P4S_COUNTER_e ct, uint32_t index, SMEMTYPE(counter)* counter, SHORT_STDPARAMS) {
    do_counter_count(counter, index, ct == enum_PSA_CounterType_t_PACKETS ? 1 : packet_size(pd));
}

void EXTERNCALL0(Direct_counter,count)(int counter_type, SMEMTYPE(direct_counter)* smem, SHORT_STDPARAMS) {
    debug("    : Executing extern_direct_counter_count\n");
}


void EXTERNCALL1(Meter,execute,i32)(uint32_t x, T4P4S_METER_e meter_type, int index, int color, SMEMTYPE(meter)* e, SHORT_STDPARAMS) {
    debug("    : Executing extern_meter_execute_meter__u32#" T4LIT(%d) "\n", index);
}

void EXTERNCALL1(Meter,execute,u32)(uint32_t index, T4P4S_METER_e b, uint32_t c, uint8_t* d, SMEMTYPE(meter)* e, SHORT_STDPARAMS) {
    debug("    : Executing extern_meter_execute_meter__u32#" T4LIT(%d) "\n", index);
}

void EXTERNCALL1(Direct_meter,read,u8)(T4P4S_METER_e b, uint32_t c, SMEMTYPE(meter)* e, SHORT_STDPARAMS) {
    debug("    : Executing extern_direct_meter_read__u8\n");
}


void EXTERNCALL0(Register,read)(uint32_t index, uint32_t a, uint32_t b, REGTYPE(uint,32) c, SHORT_STDPARAMS) {
    debug("    : Executing extern_register_read#" T4LIT(%d) "\n", index);
}

void EXTERNCALL2(Register,write,u32,u32)(uint32_t x1, uint32_t index, uint32_t value, REGTYPE(uint,32)* reg, SHORT_STDPARAMS) {
    debug(" :::: calling extern " T4LIT(Register/write/u32/i32,extern) "\n");
}

void EXTERNCALL2(Register,write,u32,i32)(uint32_t x1, int index, uint32_t value, REGTYPE(uint,32)* reg, SHORT_STDPARAMS) {
    debug(" :::: calling extern " T4LIT(Register/write/u32/i32,extern) "\n");
}

// TODO the return type should appear as a parameter like this: EXTERNIMPL1(Register,read,u32)
int EXTERNIMPL1(Register,read)(REGTYPE(uint,32)* reg, SHORT_STDPARAMS) {
    debug(" :::: calling extern " T4LIT(Register/read,extern) "\n");
    // TODO temporary implementation
    return 0;
}
