// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

// This file is included directly from `dpdk_tables.c`.


struct rte_hash* hash_create(int socketid, const char* name, uint32_t keylen, rte_hash_function hashfunc)
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
        create_error(socketid, "hash", name);
    return h;
}

void exact_create(lookup_table_t* t, int socketid)
{
    char name[64];
    snprintf(name, sizeof(name), "%d_exact_%d_%d", t->id, socketid, t->instance);
    struct rte_hash* h = hash_create(socketid, name, t->entry.key_size, rte_hash_crc);
    create_ext_table(t, h, socketid);
}

int32_t hash_add_key(struct rte_hash* h, void *key)
{
    int32_t ret;
    ret = rte_hash_add_key(h,(void *) key);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Unable to add entry to the hash.\n");
    return ret;
}

void exact_add(lookup_table_t* t, uint8_t* key, uint8_t* value)
{
    if (t->entry.key_size == 0) return; // don't add lines to keyless tables

    extended_table_t* ext = (extended_table_t*)t->table;
    uint32_t index = rte_hash_add_key(ext->rte_table, (void*) key);

    if (unlikely(index < 0))
        rte_exit(EXIT_FAILURE, "HASH: add failed\n");

    ext->content[index%t->max_size] = make_table_entry_on_socket(t, value);

    // dbg_bytes(key, t->entry.key_size, "   :: Add " T4LIT(exact) " entry to " T4LIT(%s,table) " (hash " T4LIT(%d) "): " T4LIT(%s,action) " <- ", t->name, index, get_entry_action_name(value));
}

void exact_delete(lookup_table_t* t, uint8_t* key)
{
    if (t->entry.key_size == 0) return; // nothing must have been added

    extended_table_t* ext = (extended_table_t*)t->table;
    int32_t ret = rte_hash_lookup(ext->rte_table, key);
    if (ret >= 0)
        rte_free(ext->content[ret%t->max_size]);
}

uint8_t* exact_lookup(lookup_table_t* t, uint8_t* key)
{
    if(unlikely(t->entry.key_size == 0)) return t->default_val;
    extended_table_t* ext = (extended_table_t*)t->table;
    int ret = rte_hash_lookup(ext->rte_table, key);
    return (ret < 0)? t->default_val : ext->content[ret%t->max_size];
}

void exact_flush(lookup_table_t* t)
{
    void *data, *next_key;
    uint32_t iter = 0;

    extended_table_t* ext = (extended_table_t*)t->table;
    rte_hash_reset(ext->rte_table);
    while (rte_hash_iterate(ext->rte_table, (const void**)&next_key, &data, &iter) >= 0) {
        exact_delete(ext->rte_table, next_key);
    }
}
