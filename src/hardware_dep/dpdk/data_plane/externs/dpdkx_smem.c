// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_lib.h"
#include "stateful_memory.h"

void apply_direct_smem32(rte_atomic32_t* smem_pob, uint32_t value, const char*const table_name, const char*const smem_type_name, const char*const smem_name) {
    rte_atomic32_add(smem_pob, value);
    debug("    : Applying " T4LIT(%s) " " T4LIT(%s,smem) "(" T4LIT(%d) ") on table " T4LIT(%s,table) ": new value is " T4LIT(%d) "\n",
          smem_type_name, smem_name, value, table_name, rte_atomic32_read(smem_pob));
}

void apply_direct_meter(SMEMTYPE(direct_meter)* smem, uint32_t packets_value, uint32_t bytes_value, const char*const table_name, const char*const smem_type_name, const char*const smem_name) {
    if (smem->pob == pob_packets || smem->pob == pob_packets_and_bytes)    apply_direct_smem32(&(smem->packets), packets_value, table_name, smem_type_name, smem_name);
    if (smem->pob == pob_bytes   || smem->pob == pob_packets_and_bytes)    apply_direct_smem32(&(smem->bytes),   bytes_value,   table_name, smem_type_name, smem_name);
}

void apply_direct_counter(SMEMTYPE(direct_counter)* smem, uint32_t packets_value, uint32_t bytes_value, const char*const table_name, const char*const smem_type_name, const char*const smem_name) {
    if (smem->pob == pob_packets || smem->pob == pob_packets_and_bytes)    apply_direct_smem32(&(smem->packets), packets_value, table_name, smem_type_name, smem_name);
    if (smem->pob == pob_bytes   || smem->pob == pob_packets_and_bytes)    apply_direct_smem32(&(smem->bytes),   bytes_value,   table_name, smem_type_name, smem_name);
}


void apply_direct_smem_rte_atomic32_t(rte_atomic32_t* smem, uint32_t value, const char*const table_name, const char*const smem_type_name, const char*const smem_name) {
    rte_atomic32_add(smem, value);

    debug("    : Applying " T4LIT(%s) " " T4LIT(%s,smem) "(" T4LIT(%d) ") on table " T4LIT(%s,table) ": new value is " T4LIT(%d) "\n",
          smem_type_name, smem_name, value, table_name, rte_atomic32_read(smem));
}


void do_counter_count_pob32(rte_atomic32_t* smem_pob, int index, uint32_t value, const char*const counter_name) {
    rte_atomic32_add(smem_pob, value);
    #ifdef T4P4S_DEBUG
        debug("    : Counter " T4LIT(%s[%d],smem) " += " T4LIT(%d) " = " T4LIT(%d,bytes) "\n", counter_name, index, value, rte_atomic32_read(smem_pob));
    #endif
}


void do_counter_count(SMEMTYPE(counter)* counter, int index, uint32_t value) {
    #ifdef T4P4S_DEBUG
        const char*const counter_name = counter->name;
    #else
        const char*const counter_name = NULL;
    #endif

    if (counter->pob == pob_packets || counter->pob == pob_packets_and_bytes)    do_counter_count_pob32(&(counter[index].packets), index, value, counter_name);
    if (counter->pob == pob_bytes   || counter->pob == pob_packets_and_bytes)    do_counter_count_pob32(&(counter[index].bytes), index, value, counter_name);
}


void EXTERNCALL2(Counter,count,u32,u8)() {
    debug("    : Executing EXTERNCALL2(Counter,count,u32,u8)\n");
}
void EXTERNCALL2(Counter,count,u32,u32)() {
    debug("    : Executing EXTERNCALL2(Counter,count,u32,u32)\n");
}
void EXTERNCALL2(Counter,count,i32,i32)() {
    debug("    : Executing EXTERNCALL2(Counter,count,i32,i32)\n");
}
void EXTERNCALL2(Counter,count,buf,u8)() {
    debug("    : Executing EXTERNCALL2(Counter,count,buf,u8)\n");
}

// TODO these should be autogenerated by converting the argument
// void EXTERNCALL1(Digest,pack,mac_learn_digest_data)() {
//     debug("    : Executing EXTERNCALL1(Digest,pack,mac_learn_digest_data)\n");
// }
// void EXTERNCALL1(Digest,pack,arp_digest_data)() {
//     debug("    : Executing EXTERNCALL1(Digest,pack,arp_digest_data)\n");
// }
// void EXTERNCALL1(Digest,pack,mac_learn_digest_t)() {
//     debug("    : Executing EXTERNCALL1(Digest,pack,mac_learn_digest_t)\n");
// }
// void EXTERNCALL1(Digest,pack,learn_digest_t)() {
//     debug("    : Executing EXTERNCALL1(Digest,pack,mac_learn_digest_t)\n");
// }



extern void gen_init_smems();

void init_memories() {
    debug(" :::: Init stateful memories\n");
    gen_init_smems();
}
