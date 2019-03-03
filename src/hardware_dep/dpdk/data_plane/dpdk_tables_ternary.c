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


// This file is included directly from `dpdk_tables.c`.


void ternary_create(lookup_table_t* t, int socketid)
{
    t->table = naive_ternary_create(t->entry.key_size, t->max_size);
}

void ternary_add(lookup_table_t* t, uint8_t* key, uint8_t* mask, uint8_t* value)
{
    if (t->entry.key_size == 0) return; // don't add lines to keyless tables

    uint8_t* entry = make_table_entry_on_socket(t, value);
    naive_ternary_add(t->table, key, mask, entry);
}

uint8_t* ternary_lookup(lookup_table_t* t, uint8_t* key)
{
    if (t->entry.key_size == 0) return t->default_val;
    uint8_t* ret = naive_ternary_lookup(t->table, key);
    return ret == NULL ? t->default_val : ret;
}

void ternary_flush(lookup_table_t* t)
{
    if (t->entry.key_size == 0) return; // nothing must have been added

    naive_ternary_flush(t->table);
}
