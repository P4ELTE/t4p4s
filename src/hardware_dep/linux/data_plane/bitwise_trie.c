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

#include "bitwise_trie.h"

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MSB(x) ((x) >> (8 * sizeof((x)) - 1))

struct bitwise_trie_node
{
    int value;
    struct bitwise_trie_node* parent;
    struct bitwise_trie_node* child[2];
};

struct bitwise_trie
{
    struct bitwise_trie_parameters parameters;
    struct bitwise_trie_node* root;
};

static inline struct bitwise_trie_node* allocate_node(struct bitwise_trie_node* parent, int value)
{
    struct bitwise_trie_node* const node = malloc(sizeof(*node));

    if (node != NULL)
    {
        node->value = value;
        node->parent = parent;
        node->child[0] = NULL;
        node->child[1] = NULL;
    }

    return node;
}

static unsigned int find_longest_match(const struct bitwise_trie_node* root, const void* key, unsigned int length, const struct bitwise_trie_node** lm_node, unsigned int lpm_match)
{
    const struct bitwise_trie_node* node, * next_node, * lpm_node;
    unsigned int size, bit, lpm_bit, i, j;
    uint8_t key_byte;

    node = root;
    lpm_node = root;
    size = (length + 7) / 8;
    bit = 0;
    lpm_bit = 0;

    for (i = 0; i < size; ++i)
    {
        key_byte = ((const uint8_t*)key)[i];

        for (j = 0; bit < length && j < 8; ++j)
        {
            next_node = node->child[MSB(key_byte)];

            if (node->value >= 0)
            {
                lpm_node = node;
                lpm_bit = bit;
            }

            if (next_node == NULL)
                goto found_lm;

            node = next_node;
            key_byte <<= 1;
            bit++;
        }
    }

    if (node->value >= 0)
    {
        lpm_node = node;
        lpm_bit = bit;
    }

found_lm:
    *lm_node = lpm_match ? lpm_node : node;

    return lpm_match ? lpm_bit : bit;
}

struct bitwise_trie* bitwise_trie_create(const struct bitwise_trie_parameters* parameters)
{
    struct bitwise_trie* trie;

    if (parameters == NULL || parameters->key_size == 0)
    {
        errno = EINVAL;
        return NULL;
    }

    if ((trie = malloc(sizeof(*trie))) == NULL)
        goto no_mem_trie;

    if ((trie->root = allocate_node(NULL, parameters->default_value)) == NULL)
        goto no_mem_root;

    memcpy(&trie->parameters, parameters, sizeof(*parameters));

    return trie;

no_mem_root:
    free(trie);
no_mem_trie:
    errno = ENOMEM;

    return NULL;
}

void bitwise_trie_free(struct bitwise_trie* trie)
{
    struct bitwise_trie_node* node, * tmp_node;

    if (trie == NULL)
        return;

    node = trie->root;

    if (node->child[0] == NULL && node->child[1] == NULL)
        goto free_root;

    do
    {
        if (node->child[0] != NULL)
        {
            node = node->child[0];
        }
        else if (node->child[1] != NULL)
        {
            node = node->child[1];
        }
        else
        {
            tmp_node = node;
            node = node->parent;

            if (node->child[0] == tmp_node)
                node->child[0] = NULL;
            else
                node->child[1] = NULL;

            free(tmp_node);
        }
    }
    while (node->parent != NULL);

free_root:
    free(trie->root);
    free(trie);
}

int bitwise_trie_add(struct bitwise_trie* trie, const void* key, unsigned int prefix_length, int value)
{
    const struct bitwise_trie_node* lm_node;
    struct bitwise_trie_node* node, * next_node;
    unsigned int prefix_size, bit, i, j;
    uint8_t key_byte;

    if (trie == NULL || (key == NULL && prefix_length != 0) || prefix_length > (8 * trie->parameters.key_size) || value < 0)
    {
        errno = EINVAL;
        return -1;
    }

    bit = find_longest_match(trie->root, key, prefix_length, &lm_node, 0);

    if ((prefix_length == 0 && lm_node->value != trie->parameters.default_value) ||
        (prefix_length != 0 && bit == prefix_length))
    {
        errno = EEXIST;
        return -1;
    }

    node = (struct bitwise_trie_node*)lm_node;

    if (prefix_length == 0)
        goto set_value;

    i = bit / 8;
    j = bit & 0x07u;
    prefix_size = (prefix_length + 7) / 8;
    key_byte = ((const uint8_t*)key)[i] << j;

    goto resume;

    for (; i < prefix_size; ++i)
    {
        key_byte = ((const uint8_t*)key)[i];

        for (j = 0; bit < prefix_length && j < 8; ++j)
        {
resume:     if ((next_node = allocate_node(node, -1)) == NULL)
                goto no_mem;

            node->child[MSB(key_byte)] = next_node;

            node = next_node;
            key_byte <<= 1;
            bit++;
        }
    }

set_value:
    node->value = value;

    return 0;

no_mem:
    next_node = node;
    while (next_node != lm_node)
    {
        node = next_node;
        next_node = next_node->parent;

        free(node);
    }

    errno = ENOMEM;

    return -1;
}

int bitwise_trie_remove(struct bitwise_trie* trie, const void* key, unsigned int prefix_length)
{
    const struct bitwise_trie_node* lpm_node;
    struct bitwise_trie_node* node, * tmp_node;
    unsigned int bit;

    if (trie == NULL || (key == NULL && prefix_length != 0) || prefix_length > (8 * trie->parameters.key_size))
    {
        errno = EINVAL;
        return -1;
    }

    bit = find_longest_match(trie->root, key, prefix_length, &lpm_node, 1);

    if (bit != prefix_length)
    {
        errno = ENOENT;
        return -1;
    }

    tmp_node = (struct bitwise_trie_node*)lpm_node;

    if (prefix_length == 0 || tmp_node->child[0] != NULL || tmp_node->child[1] != NULL)
    {
        tmp_node->value = prefix_length == 0 ? trie->parameters.default_value : -1;
        return 0;
    }

    node = tmp_node->parent;
    free(tmp_node);

    while (node->parent != NULL && node->value < 0 && (node->child[0] != NULL) ^ (node->child[1] != NULL))
    {
        tmp_node = node;
        node = node->parent;

        free(tmp_node);
    }

    if (node->child[0] == tmp_node)
        node->child[0] = NULL;
    else
        node->child[1] = NULL;

    return 0;
}

int bitwise_trie_lookup(const struct bitwise_trie* trie, const void* key)
{
    const struct bitwise_trie_node* lpm_node;

    if (trie == NULL || key == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    find_longest_match(trie->root, key, 8 * trie->parameters.key_size, &lpm_node, 1);

    return lpm_node->value;
}

int bitwise_trie_default_value(const struct bitwise_trie* trie)
{
    return trie->parameters.default_value;
}
