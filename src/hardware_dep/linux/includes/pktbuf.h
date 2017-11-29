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

#ifndef PKTBUF_H
#define PKTBUF_H

#include <stddef.h>

struct pktbuf
{
    void* buffer;
    size_t buffer_size;
    void* data;
    size_t data_size;
    void* pointer;
};

void pktbuf_use(struct pktbuf* p, void* buffer, size_t buffer_size);
void pktbuf_use_with_headroom(struct pktbuf* p, void* buffer, size_t buffer_size, size_t headroom);
void pktbuf_use_data(struct pktbuf* p, void* data, size_t data_size);
void pktbuf_use_data_with_headroom(struct pktbuf* p, void* data, size_t data_size, size_t headroom);

void pktbuf_init(struct pktbuf* p, size_t buffer_size);
void pktbuf_init_with_headroom(struct pktbuf* p, size_t buffer_size, size_t headroom);
void pktbuf_uninit(struct pktbuf* p);

void* pktbuf_at(const struct pktbuf* p, size_t offset, size_t size);
void* pktbuf_head(const struct pktbuf* p);
void* pktbuf_data(const struct pktbuf* p);
void* pktbuf_tail(const struct pktbuf* p);
void* pktbuf_back(const struct pktbuf* p);

size_t pktbuf_size(const struct pktbuf* p);
size_t pktbuf_headroom(const struct pktbuf* p);
size_t pktbuf_tailroom(const struct pktbuf* p);

void* pktbuf_copy_data(const struct pktbuf* src, struct pktbuf* dst);
void* pktbuf_set_data(struct pktbuf* p, const void* data, size_t size);

void* pktbuf_head_push(struct pktbuf* p, size_t size);
void* pktbuf_head_pop(struct pktbuf* p, size_t size);
void* pktbuf_tail_push(struct pktbuf* p, size_t size);
void* pktbuf_tail_pop(struct pktbuf* p, size_t size);

#endif
