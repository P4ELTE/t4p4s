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

#ifndef BITWISE_TRIE_H
#define BITWISE_TRIE_H

struct bitwise_trie;

struct bitwise_trie_parameters
{
    int default_value;
    unsigned int key_size;
};

struct bitwise_trie* bitwise_trie_create(const struct bitwise_trie_parameters* parameters);
void bitwise_trie_free(struct bitwise_trie* trie);

int bitwise_trie_add(struct bitwise_trie* trie, const void* key, unsigned int prefix_length, int value);
int bitwise_trie_remove(struct bitwise_trie* trie, const void* key, unsigned int prefix_length);
int bitwise_trie_lookup(const struct bitwise_trie* trie, const void* key);

int bitwise_trie_default_value(const struct bitwise_trie* trie);

#endif
