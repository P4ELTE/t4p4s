#ifndef TERNARY_NAIVE_H
#define TERNARY_NAIVE_H

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t* mask;
    uint8_t* key;
    uint8_t* value;
} ternary_entry;

typedef struct {
    void**  entries;
    uint8_t keylen;
    uint8_t size;
} ternary_table;

ternary_table* naive_ternary_create (uint8_t keylen, uint8_t max_size);
void           naive_ternary_destroy(ternary_table* t);
void           naive_ternary_add    (ternary_table* t, uint8_t* key, uint8_t* mask, uint8_t* value);
uint8_t*       naive_ternary_lookup (ternary_table* t, uint8_t* key);

#endif
