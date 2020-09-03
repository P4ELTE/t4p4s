// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#include "backend.h"
#include "dpdk_tables.h"
#include "util.h"

#include "tables.h"

extern char* action_names[];

// ============================================================================
// LOOKUP TABLE IMPLEMENTATIONS

#include <rte_hash.h>       // EXACT
#include <rte_hash_crc.h>
#include <nmmintrin.h> 
#include <rte_lpm.h>        // LPM (32 bit key)
#include <rte_lpm6.h>       // LPM (128 bit key)
#include "ternary_naive.h"  // TERNARY

#include <rte_malloc.h>     // extended tables
#include <rte_errno.h>

// ============================================================================
// Getters

// Returns the action id stored in the table entry parameter.
// Table entries have different types (${table.name}_action),
// but all of them have to start with an int, the action id.
char* get_entry_action_name(void* entry) {
    int action_id = *((int*)entry);
    return action_names[action_id];
}

// Computes the location of the validity field of the entry.
bool* entry_validity_ptr(uint8_t* entry, lookup_table_t* t) {
    return (bool*)(entry + t->entry.action_size + t->entry.state_size);
}

// ============================================================================
// Error messages

void create_error_text(int socketid, const char* errno_txt, const char* table_type, const char* table_name, const char* error_text)
{
    rte_exit(EXIT_FAILURE, "DPDK (errno=" T4LIT(rte_errno,error) ", " T4LIT(%s,error) "): Unable to create %s table " T4LIT(%s,table) " on socket " T4LIT(%d,socket) ": %s\n", errno_txt, table_type, table_name, socketid, error_text);
}

void create_error(int socketid, const char* table_type, const char* table_name)
{
    if (rte_errno == E_RTE_NO_CONFIG) {
        create_error_text(socketid, "E_RTE_NO_CONFIG", table_type, table_name, "function could not get pointer to rte_config structure");
    }
    if (rte_errno == E_RTE_SECONDARY) {
        create_error_text(socketid, "E_RTE_SECONDARY", table_type, table_name, "function was called from a secondary process instance");
    }
    if (rte_errno == ENOENT) {
        create_error_text(socketid, "ENOENT", table_type, table_name, "missing entry");
    }
    if (rte_errno == EINVAL) {
        create_error_text(socketid, "EINVAL", table_type, table_name, "invalid parameter passed to function");
    }
    if (rte_errno == ENOSPC) {
        create_error_text(socketid, "ENOSPC", table_type, table_name, "the maximum number of memzones has already been allocated");
    }
    if (rte_errno == EEXIST) {
        create_error_text(socketid, "EEXIST", table_type, table_name, "a memzone with the same name already exists");
    }
    if (rte_errno == ENOMEM) {
        create_error_text(socketid, "ENOMEM", table_type, table_name, "no appropriate memory area found in which to create memzone");
    }
}

// ============================================================================
// Table entry creation

// Sets up the fields of a table entry.
void make_table_entry(uint8_t* entry, uint8_t* value, lookup_table_t* t) {
    memcpy(entry, value, t->entry.action_size);
    memset(entry + t->entry.action_size, 0, t->entry.state_size);
    *entry_validity_ptr(entry, t) = VALID_TABLE_ENTRY;
}

uint8_t* make_table_entry_on_socket(lookup_table_t* t, uint8_t* value) {
    int length = t->entry.entry_size;
    uint8_t* entry = rte_malloc_socket("uint8_t", sizeof(uint8_t)*length, 0, t->socketid);
    if (unlikely(entry == NULL)) {
        create_error(-1, t->type == 0 ? "hash" : t->type == 1 ? "lpm" : "ternary", t->name);
    }
    make_table_entry(entry, value, t);
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
        create_error(-1, t->type == 0 ? "hash" : t->type == 1 ? "lpm" : "ternary", t->name);
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
        case LOOKUP_EXACT:
            exact_create(t, socketid);
            break;
        case LOOKUP_LPM:
            lpm_create(t, socketid);
            break;
        case LOOKUP_TERNARY:
            ternary_create(t, socketid);
            break;
    }
}

void flush_table(lookup_table_t* t)
{
    if (t->entry.key_size == 0) return; // must be a fake table

    switch(t->type)
    {
        case LOOKUP_EXACT:
            exact_flush(t);
            break;
        case LOOKUP_LPM:
            lpm_flush(t);
            break;
        case LOOKUP_TERNARY:
            ternary_flush(t);
            break;
    }
}

void table_set_default_action(lookup_table_t* t, uint8_t* entry)
{
    if (t->default_val) rte_free(t->default_val);

    t->default_val = make_table_entry_on_socket(t, entry);
    *entry_validity_ptr(t->default_val, t) = INVALID_TABLE_ENTRY;
}
