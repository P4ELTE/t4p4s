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

#include "linux_backend.h"
#include "pktbuf.h"
#include "vector.h"

#include <stdint.h>
#include <stdlib.h>

uint32_t read_counter(int id, int index)
{
    return *(uint32_t*)vector_get(counters[id]->values, index);
}

void increase_counter(int id, int index)
{
    uint32_t* const counter = vector_get(counters[id]->values, index);
    (*counter)++;
}

void read_register(int id, int index, uint8_t* dst)
{
    p4_register_t* const r = registers[id];

    memcpy(dst, &r->values[index * (r->width)], r->width);
}

void write_register(int id, int index, uint8_t* src)
{
    p4_register_t* const r = registers[id];

    memcpy(&r->values[index * (r->width)], src, r->width);
}

void push(packet_descriptor_t* pd, header_stack_t h)
{
    unsigned int i, next = 0;

    while (next < header_stack_size[h] && pd->headers[header_stack_elements[h][next]].pointer != NULL)
        next++;

    for (i = 0; i < next; ++i)
        pd->headers[header_stack_elements[h][i + 1]].pointer = pd->headers[header_stack_elements[h][i]].pointer;

    pd->headers[header_stack_elements[h][0]].pointer = pktbuf_head_push(pd->wrapper, pd->headers[header_stack_elements[h][0]].length);
}

void pop(packet_descriptor_t* pd, header_stack_t h)
{
    unsigned int i, last = 0;

    while (last < header_stack_size[h] && pd->headers[header_stack_elements[h][last]].pointer != NULL)
        last++;

    if (last != 0)
    {
        last--;

        for (i = 0; i < last; ++i)
            pd->headers[header_stack_elements[h][i]].pointer = pd->headers[header_stack_elements[h][i + 1]].pointer;

        pd->headers[header_stack_elements[h][last]].pointer = NULL;
    }
}

void add_header(packet_descriptor_t* pd, header_reference_t h)
{
    if (pd->headers[h.header_instance].pointer == NULL)
        pd->headers[h.header_instance].pointer = pktbuf_head_push(pd->wrapper, h.bytewidth);
}

void remove_header(packet_descriptor_t* pd, header_reference_t h)
{
    pd->headers[h.header_instance].pointer = NULL;
}

void drop(packet_descriptor_t* pd)
{
    pd->dropped = 1;
}

void generate_digest(backend bg, char* name, uint32_t receiver, struct type_field_list* digest_field_list)
{
    digest d = create_digest(bg, name);
    int i;

    for (i = 0; i < digest_field_list->fields_quantity; ++i)
        d = add_digest_field(d, digest_field_list->field_offsets[i], digest_field_list->field_widths[i]);

    send_digest(bg, d, receiver);
}

void no_op(void)
{

}
