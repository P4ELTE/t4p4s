// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#include "backend.h"
#include "dpdk_tables.h"
#include "util_debug.h"

#include "tables.h"

// ============================================================================
// LOOKUP TABLE IMPLEMENTATIONS

#include <rte_hash.h>       // EXACT
#include <rte_hash_crc.h>

#ifndef __aarch64__
#include <nmmintrin.h>
#endif

#include <rte_lpm.h>        // LPM (32 bit key)
#include <rte_lpm6.h>       // LPM (128 bit key)
#include "ternary_naive.h"  // TERNARY

#include <rte_malloc.h>     // extended tables
#include <rte_errno.h>


// ============================================================================
// Error messages

void rte_exit_with_errno_text(const char* errno_txt, const char* msg, const char* table_name, const char* error_text)
{
    rte_exit(EXIT_FAILURE, "DPDK (errno=" T4LIT(%d,error) ", " T4LIT(%s,error) "): Unable to %s " T4LIT(%s,table) ": %s\n", rte_errno, errno_txt, msg, table_name, error_text);
}

void rte_exit_with_errno(const char* msg, const char* table_name)
{
    if (rte_errno == E_RTE_NO_CONFIG) {
        rte_exit_with_errno_text("E_RTE_NO_CONFIG", msg, table_name, "function could not get pointer to rte_config structure");
    }
    if (rte_errno == E_RTE_SECONDARY) {
        rte_exit_with_errno_text("E_RTE_SECONDARY", msg, table_name, "function was called from a secondary process instance");
    }
    if (rte_errno == ENOENT) {
        rte_exit_with_errno_text("ENOENT", msg, table_name, "missing entry");
    }
    if (rte_errno == EINVAL) {
        rte_exit_with_errno_text("EINVAL", msg, table_name, "invalid parameter passed to function");
    }
    if (rte_errno == ENOSPC) {
        rte_exit_with_errno_text("ENOSPC", msg, table_name, "the maximum number of memzones has already been allocated");
    }
    if (rte_errno == EEXIST) {
        rte_exit_with_errno_text("EEXIST", msg, table_name, "a memzone with the same name already exists");
    }
    if (rte_errno == ENOMEM) {
        rte_exit_with_errno_text("ENOMEM", msg, table_name, "no appropriate memory area found in which to create memzone");
    }
}

// ============================================================================
// Table entry creation

// Sets up the fields of a table entry.
void fill_table_entry(uint8_t* entry, const base_table_action_t*const action, lookup_table_t* t) {
    memcpy(entry, action, t->entry.action_size);
    memset(entry + t->entry.action_size, 0, t->entry.state_size);
}

uint8_t* make_table_entry(lookup_table_t* t, const base_table_action_t*const action) {
    int length = t->entry.action_size + t->entry.state_size;
    uint8_t* entry = rte_malloc_socket("uint8_t", sizeof(uint8_t)*length, 0, t->socketid);
    if (unlikely(entry == NULL)) {
        rte_exit_with_errno(t->type == 0 ? "create hash table" : t->type == 1 ? "create lpm table" : "create ternary table", t->short_name);
    }
    fill_table_entry(entry, action, t);
    return entry;
}

// ============================================================================
// Table implementations

void create_ext_table(lookup_table_t* t, void* rte_table, int socketid);

#include "dpdk_tables_exact.c"
#include "dpdk_tables_lpm.c"
#include "dpdk_tables_ternary.c"

// ============================================================================
// HIGHER LEVEL TABLE MANAGEMENT

void create_ext_table(lookup_table_t* t, void* rte_table, int socketid)
{
    extended_table_t* ext = rte_malloc_socket("extended_table_t", sizeof(extended_table_t), 0, socketid);
    ext->rte_table = rte_table;
    ext->size = 0;
    ext->content = rte_malloc_socket("uint8_t*", sizeof(uint8_t*)*t->max_size, 0, socketid);
    if (unlikely(ext->content == NULL)) {
        rte_exit_with_errno(t->type == 0 ? "create hash table" : t->type == 1 ? "create lpm table" : "cretate ternary table", t->canonical_name);
    }
    t->table = ext;
}

void create_table(lookup_table_t* t, int socketid)
{
    t->socketid = socketid;
    t->default_val = 0;
    if (t->entry.key_size == 0) return; // we don't create the table if there are no keys (it's a fake table for an element in the pipeline)

    switch(t->type)
    {
        case LOOKUP_exact:
            exact_create(t, socketid);
            break;
        case LOOKUP_lpm:
            lpm_create(t, socketid);
            break;
        case LOOKUP_ternary:
            ternary_create(t, socketid);
            break;
    }
}

void flush_table(lookup_table_t* t)
{
    if (t->entry.key_size == 0) return; // must be a fake table

    switch(t->type)
    {
        case LOOKUP_exact:
            exact_flush(t);
            break;
        case LOOKUP_lpm:
            lpm_flush(t);
            break;
        case LOOKUP_ternary:
            ternary_flush(t);
            break;
    }
}

void table_set_default_action(lookup_table_t* t, ENTRYBASE* entry)
{
    if (t->default_val) rte_free(t->default_val);

    t->default_val = make_table_entry(t, entry);
}
