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

#include <rte_mempool.h>
#include <rte_mbuf.h>

// TODO push N elements
void
push(packet_descriptor_t* p, header_stack_t h)
{
    int next = 0;
    while(next < header_stack_size[h] && p->headers[header_stack_elements[h][next]].pointer != NULL) next++;
    debug("pushing next %d\n", next);
    int i;
    for(i = 0; i < next; i++)
        p->headers[header_stack_elements[h][i+1]].pointer = p->headers[header_stack_elements[h][i]].pointer;
    p->headers[header_stack_elements[h][0]].pointer =
        rte_pktmbuf_prepend(p->wrapper, p->headers[header_stack_elements[h][0]].length);
}

// TODO pop N elements
void
pop(packet_descriptor_t* p, header_stack_t h)
{
    int last = 0;
    while(last < header_stack_size[h] && p->headers[header_stack_elements[h][last]].pointer != NULL) last++;
    if(last > 0) {
        last--;
        debug("popping last %d\n", last);
        int i;
        for(i = 0; i < last; i++)
            p->headers[header_stack_elements[h][i]].pointer = p->headers[header_stack_elements[h][i+1]].pointer;
        // TODO: free up the corresponding part of the mbuf (rte_pktmbuf_adj is not appropriate here)
        p->headers[header_stack_elements[h][last]].pointer = NULL;
    }
    else debug("popping from empty header stack...\n");
}

void
add_header(packet_descriptor_t* p, header_reference_t h)
{
    if(p->headers[h.header_instance].pointer == NULL)
        p->headers[h.header_instance].pointer = rte_pktmbuf_prepend(p->wrapper, h.bytewidth);
    else
        debug("Cannot add a header instance already present in the packet\n");
}

void
remove_header(packet_descriptor_t* p, header_reference_t h)
{
    if(p->headers[h.header_instance].pointer != NULL) {
        // TODO: free up the corresponding part of the mbuf
        p->headers[h.header_instance].pointer = NULL;
    } else {
        debug("Cannot remove a header instance not present in the packet\n");
    }
}

void 
generate_digest(backend bg, char* name, int receiver, struct type_field_list* digest_field_list)
{
    digest d = create_digest(bg, name);
    int i;
    for(i = 0; i < digest_field_list->fields_quantity; i++)
        d = add_digest_field(d, digest_field_list->field_offsets[i], digest_field_list->field_widths[i]);
    send_digest(bg, d, receiver);
}

void no_op()
{
}

void drop(packet_descriptor_t* p)
{
    // TODO
}
