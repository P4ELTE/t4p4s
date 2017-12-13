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

#include "pktbuf.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void pktbuf_use(struct pktbuf* p, void* buffer, size_t buffer_size)
{
    pktbuf_use_with_headroom(p, buffer, buffer_size, 0);
}

void pktbuf_use_with_headroom(struct pktbuf* p, void* buffer, size_t buffer_size, size_t headroom)
{
    assert(buffer != NULL);

    p->buffer = buffer;
    p->buffer_size = buffer_size;
    p->data = (uint8_t*)buffer + headroom;
    p->data_size = 0;
    p->pointer = NULL;
}

void pktbuf_use_data(struct pktbuf* p, void* data, size_t data_size)
{
    pktbuf_use_data_with_headroom(p, data, data_size, 0);
}

void pktbuf_use_data_with_headroom(struct pktbuf* p, void* data, size_t data_size, size_t headroom)
{
    assert(data != NULL);

    p->buffer = (uint8_t*)data - headroom;
    p->buffer_size = data_size + headroom;
    p->data = data;
    p->data_size = data_size;
    p->pointer = NULL;
}

void pktbuf_init(struct pktbuf* p, size_t buffer_size)
{
    pktbuf_use(p, malloc(buffer_size), buffer_size);
}

void pktbuf_init_with_headroom(struct pktbuf* p, size_t buffer_size, size_t headroom)
{
    pktbuf_use_with_headroom(p, malloc(buffer_size), buffer_size, headroom);
}

void pktbuf_uninit(struct pktbuf* p)
{  
    free(p->buffer);
}

void* pktbuf_at(const struct pktbuf* p, size_t offset, size_t size)
{
    assert(offset + size <= p->data_size);

    return (uint8_t*)p->data + offset;
}

void* pktbuf_head(const struct pktbuf* p)
{
    return p->buffer;
}

void* pktbuf_data(const struct pktbuf* p)
{
    return p->data;
}

void* pktbuf_tail(const struct pktbuf* p)
{
    return (uint8_t*)p->data + p->data_size;
}

void* pktbuf_back(const struct pktbuf* p)
{
    return (uint8_t*)p->buffer + p->buffer_size;
}

size_t pktbuf_size(const struct pktbuf* p)
{
    return p->buffer_size - pktbuf_headroom(p);
}

size_t pktbuf_headroom(const struct pktbuf* p)
{
    return (uint8_t*)p->data - (uint8_t*)p->buffer;
}

size_t pktbuf_tailroom(const struct pktbuf* p)
{
    return (uint8_t*)pktbuf_back(p) - (uint8_t*)pktbuf_tail(p);
}

void* pktbuf_copy_data(const struct pktbuf* src, struct pktbuf* dst)
{
    assert(pktbuf_size(dst) >= src->data_size);

    memcpy(dst->data, src->data, src->data_size);
    dst->data_size = src->data_size;

    return dst->data;
}

void* pktbuf_set_data(struct pktbuf* p, const void* data, size_t size)
{
    assert(pktbuf_size(p) >= size);

    memcpy(p->data, data, size);
    p->data_size = size;

    return p->data;
}

void* pktbuf_head_push(struct pktbuf* p, size_t size)
{
    assert(pktbuf_headroom(p) >= size);

    p->data = (uint8_t*)p->data - size;
    p->data_size += size;

    return p->data;
}

void* pktbuf_head_pop(struct pktbuf* p, size_t size)
{
    assert(p->data_size >= size);

    p->data = (uint8_t*)p->data + size;
    p->data_size -= size;

    return p->data;
}

void* pktbuf_tail_push(struct pktbuf* p, size_t size)
{
    assert(pktbuf_tailroom(p) >= size);

    void* const tail = pktbuf_tail(p);
    p->data_size += size;

    return tail;
}

void* pktbuf_tail_pop(struct pktbuf* p, size_t size)
{
    assert(p->data_size >= size);

    p->data_size -= size;

    return pktbuf_tail(p);
}
