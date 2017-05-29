// Copyright 2016 Eotvos Lorand University, Budapest, Hungary
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "backend.h"
#include "dataplane.h"
#include "dpdk_tables.h"

// ============================================================================
// LOOKUP TABLE IMPLEMENTATIONS

#include <rte_hash.h>       // EXACT
#include <rte_hash_crc.h>
#include <nmmintrin.h> 
#include <rte_lpm.h>        // LPM (32 bit key)
#include <rte_lpm6.h>       // LPM (128 bit key)
#include "ternary_naive.h"  // TERNARY

#include <rte_malloc.h>     // extended tables
#include <rte_version.h>    // for conditional on rte_hash parameters
#include <rte_errno.h>

static uint32_t crc32(const void *data, uint32_t data_len, uint32_t init_val) {
    int32_t *data32 = (void*)data;
    uint32_t result = init_val; 
    result = _mm_crc32_u32 (result, *data32++);
    return result;
}

static uint8_t*
copy_to_socket(uint8_t* src, int length, int socketid) {
    uint8_t* dst = rte_malloc_socket("uint8_t", sizeof(uint8_t)*length, 0, socketid);
    memcpy(dst, src, length);
    return dst;
}

// ============================================================================
// LOWER LEVEL TABLE MANAGEMENT

void create_error_text(int socketid, char* table_type, char* error_text)
{
    rte_exit(EXIT_FAILURE, "DPDK: Unable to create the %s on socket %d: %s\n", table_type, socketid, error_text);
}

void create_error(int socketid, char* table_type)
{
    if (rte_errno == E_RTE_NO_CONFIG) {
        create_error_text(socketid, table_type, "function could not get pointer to rte_config structure");
    }
    if (rte_errno == E_RTE_SECONDARY) {
        create_error_text(socketid, table_type, "function was called from a secondary process instance");
    }
    if (rte_errno == ENOENT) {
        create_error_text(socketid, table_type, "missing entry");
    }
    if (rte_errno == EINVAL) {
        create_error_text(socketid, table_type, "invalid parameter passed to function");
    }
    if (rte_errno == ENOSPC) {
        create_error_text(socketid, table_type, "the maximum number of memzones has already been allocated");
    }
    if (rte_errno == EEXIST) {
        create_error_text(socketid, table_type, "a memzone with the same name already exists");
    }
    if (rte_errno == ENOMEM) {
        create_error_text(socketid, table_type, "no appropriate memory area found in which to create memzone");
    }
}

struct rte_hash *
hash_create(int socketid, const char* name, uint32_t keylen, rte_hash_function hashfunc)
{
    struct rte_hash_parameters hash_params = {
        .name = NULL,
        .entries = HASH_ENTRIES,
#if RTE_VER_MAJOR == 2 && RTE_VER_MINOR == 0
        .bucket_entries = 4,
#endif
        .key_len = keylen,
        .hash_func = hashfunc,
        .hash_func_init_val = 0,
    };
    hash_params.name = name;
    hash_params.socket_id = socketid;
    struct rte_hash *h = rte_hash_create(&hash_params);
    if (h == NULL)
        create_error(socketid, "hash");
    return h;
}

struct rte_lpm *
lpm4_create(int socketid, const char* name, uint8_t max_size)
{
#if RTE_VERSION >= RTE_VERSION_NUM(16,04,0,0)
    struct rte_lpm_config config = {
        .max_rules = max_size,
        .number_tbl8s = (1 << 8), // TODO refine this
        .flags = 0
    };
    struct rte_lpm *l = rte_lpm_create(name, socketid, &config);
#else
    struct rte_lpm *l = rte_lpm_create(name, socketid, max_size, 0/*flags*/);
#endif
    if (l == NULL)
        create_error(socketid, "LPM");
    return l;
}

struct rte_lpm6 *
lpm6_create(int socketid, const char* name, uint8_t max_size)
{
    struct rte_lpm6_config config = {
        .max_rules = max_size,
        .number_tbl8s = (1 << 16),
        .flags = 0
    };
    struct rte_lpm6 *l = rte_lpm6_create(name, socketid, &config);
    if (l == NULL)
        create_error(socketid, "LPM6");
    return l;
}

int32_t
hash_add_key(struct rte_hash* h, void *key)
{
    int32_t ret;
    ret = rte_hash_add_key(h,(void *) key);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Unable to add entry to the hash.\n");
    return ret;
}

void
lpm4_add(struct rte_lpm* l, uint32_t key, uint8_t depth, uint8_t value)
{
    int ret = rte_lpm_add(l, key, depth, value);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Unable to add entry to the LPM table\n");
    debug("LPM: Added 0x%08x / %d (%d)\n", (unsigned)key, depth, value);
}

void
lpm6_add(struct rte_lpm6* l, uint8_t key[16], uint8_t depth, uint8_t value)
{
    int ret = rte_lpm6_add(l, key, depth, value);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Unable to add entry to the LPM table\n");
    debug("LPM: Adding route %s / %d (%d)\n", "IPV6", depth, value);
}

// ============================================================================
// HIGHER LEVEL TABLE MANAGEMENT

// ----------------------------------------------------------------------------
// CREATE

static void
create_ext_table(lookup_table_t* t, void* rte_table, int socketid)
{
    extended_table_t* ext = rte_malloc_socket("extended_table_t", sizeof(extended_table_t), 0, socketid);
    ext->rte_table = rte_table;
    ext->size = 0;
    ext->content = rte_malloc_socket("uint8_t*", sizeof(uint8_t*)*TABLE_MAX, 0, socketid);
    t->table = ext;
}

void
exact_create(lookup_table_t* t, int socketid)
{
    char name[64];
    snprintf(name, sizeof(name), "%d_exact_%d_%d", t->id, socketid, t->instance);
    struct rte_hash* h = hash_create(socketid, name, t->key_size, rte_hash_crc);
    create_ext_table(t, h, socketid);
}

void
lpm_create(lookup_table_t* t, int socketid)
{
    char name[64];
    snprintf(name, sizeof(name), "%d_lpm_%d_%d", t->id, socketid, t->instance);
    if(t->key_size <= 4)
        create_ext_table(t, lpm4_create(socketid, name, t->max_size), socketid);
    else if(t->key_size <= 16)
        create_ext_table(t, lpm6_create(socketid, name, t->max_size), socketid);
    else
        rte_exit(EXIT_FAILURE, "LPM: key_size not supported\n");

}

void
ternary_create(lookup_table_t* t, int socketid)
{
    t->table = naive_ternary_create(t->key_size, t->max_size);
}

// ----------------------------------------------------------------------------
// ADD

static uint8_t* add_index(uint8_t* value, int val_size, int index)
{
    // realloc doesn't work in this case ("invalid old size")
    uint8_t* value2 = malloc(val_size+sizeof(int));
    memcpy(value2, value, val_size);
    *(int*)(value2+val_size) = index;
    return value2;
}

void
exact_add(lookup_table_t* t, uint8_t* key, uint8_t* value)
{
    if(t->key_size == 0) return; // don't add lines to keyless tables
    extended_table_t* ext = (extended_table_t*)t->table;
    uint32_t index = rte_hash_add_key(ext->rte_table, (void*) key);
    if(index < 0)
        rte_exit(EXIT_FAILURE, "HASH: add failed\n");
    value = add_index(value, t->val_size, t->counter++);
    ext->content[index%256] = copy_to_socket(value, t->val_size+sizeof(int), t->socketid);
}

void
lpm_add(lookup_table_t* t, uint8_t* key, uint8_t depth, uint8_t* value)
{
    if(t->key_size == 0) return; // don't add lines to keyless tables
    extended_table_t* ext = (extended_table_t*)t->table;
    value = add_index(value, t->val_size, t->counter++);
    if(t->key_size <= 4)
    {
        ext->content[ext->size] = copy_to_socket(value, t->val_size+sizeof(int), t->socketid);

        // the rest is zeroed in case of keys smaller then 4 bytes
        uint32_t key32 = 0;
        memcpy(&key32, key, t->key_size);

        lpm4_add(ext->rte_table, key32, depth, ext->size++);
    }
    else if(t->key_size <= 16)
    {
        ext->content[ext->size] = copy_to_socket(value, t->val_size+sizeof(int), t->socketid);

        static uint8_t key128[16];
        memset(key128, 0, 16);
        memcpy(key128, key, t->key_size);

        lpm6_add(ext->rte_table, key128, depth, ext->size++);
    }
}

void
ternary_add(lookup_table_t* t, uint8_t* key, uint8_t* mask, uint8_t* value)
{
    if(t->key_size == 0) return; // don't add lines to keyless tables
    value = add_index(value, t->val_size, t->counter++);
    naive_ternary_add(t->table, key, mask, copy_to_socket(value, t->val_size+sizeof(int), t->socketid));
}

// ----------------------------------------------------------------------------
// SET DEFAULT VALUE

void
table_setdefault(lookup_table_t* t, uint8_t* value)
{
    debug("Default value set for table %s (on socket %d).\n", t->name, t->socketid);
    value = add_index(value, t->val_size, DEFAULT_ACTION_INDEX);
    if(t->default_val) rte_free(t->default_val);
    t->default_val = copy_to_socket(value, t->val_size+sizeof(int), t->socketid);
}

// ----------------------------------------------------------------------------
// LOOKUP

uint8_t*
exact_lookup(lookup_table_t* t, uint8_t* key)
{
    if(t->key_size == 0) return t->default_val;
    extended_table_t* ext = (extended_table_t*)t->table;
    int ret = rte_hash_lookup(ext->rte_table, key);
    return (ret < 0)? t->default_val : ext->content[ret % 256];
}

uint8_t*
lpm_lookup(lookup_table_t* t, uint8_t* key)
{
    if (t->key_size == 0) return t->default_val;
    extended_table_t* ext = (extended_table_t*)t->table;

    if(t->key_size <= 4)
    {
        uint32_t key32 = 0;
        memcpy(&key32, key, t->key_size);

        uint8_t result;
#if RTE_VERSION >= RTE_VERSION_NUM(16,04,0,0)
        uint32_t result32;
        int ret = rte_lpm_lookup(ext->rte_table, key32, &result32);
        result = (uint8_t)result32;
#else
        int ret = rte_lpm_lookup(ext->rte_table, key32, &result);
#endif
        return ret == 0 ? ext->content[result] : t->default_val;
    }
    else if(t->key_size <= 16)
    {
        static uint8_t key128[16];
        memset(key128, 0, 16);
        memcpy(key128, key, t->key_size);

        uint8_t result;
#if RTE_VERSION < RTE_VERSION_NUM(17, 05, 0, 0)
        // note: DPDK 17.05 changed next_hop to 32 bits
        int ret = rte_lpm6_lookup(ext->rte_table, key128, &result);
#else
        int ret = rte_lpm6_lookup_v20(ext->rte_table, key128, &result);
#endif
        return ret == 0 ? ext->content[result] : t->default_val;
    }
    return NULL;
}

uint8_t*
ternary_lookup(lookup_table_t* t, uint8_t* key)
{
    if(t->key_size == 0) return t->default_val;
    uint8_t* ret = naive_ternary_lookup(t->table, key);
    return ret == NULL ? t->default_val : ret;
}
