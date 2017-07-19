#include "ternary_naive.h"

ternary_table*
naive_ternary_create(uint8_t keylen, uint8_t max_size)
{
    ternary_table* t = malloc(sizeof(ternary_table));
    t->entries = malloc(sizeof(ternary_entry)*max_size);
    t->keylen = keylen;
    t->size = 0;
    return t;
}

void
naive_ternary_destroy(ternary_table* t)
{
    // TODO
    free(t->entries);
    free(t);
}

void
naive_ternary_add(ternary_table* t, uint8_t* key, uint8_t* mask, uint8_t* value)
{
    ternary_entry* e = malloc(sizeof(ternary_entry));
    e->key = malloc(t->keylen);
    e->mask = malloc(t->keylen);
    memcpy(e->key, key, t->keylen);
    memcpy(e->mask, mask, t->keylen);
    e->value = value;
    t->entries[t->size++] = e;
}

uint8_t*
naive_ternary_lookup(ternary_table* t, uint8_t* key)
{
    int i, j, match=1;
    ternary_entry* e;
    ternary_entry* res = NULL;
    for(i = 0; i < t->size; i++)
    {
        e = t->entries[i];
        // if(e->priority >= min_priority) continue;
        match = 1;
        for(j = 0; j < t->keylen; j++) {
            if(e->key[j] != (key[j] & e->mask[j])) {
                match = 0;
                break;
            }
        }
        if(match) res = e;
    }
    return match ? res->value : NULL;
}

