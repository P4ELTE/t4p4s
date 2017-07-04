#include "dataplane.h"
#include "backend.h"
#include "freescale_primitives.h"
#include <odp.h>

void
push(packet_descriptor_t* p, header_stack_t h) {
    uint8_t next = 0;
    while (next < header_stack_size[h] && p->headers[header_stack_elements[h][next]].pointer != NULL) next++;
    debug("pushing next %d\n", next);
    int i;
    for (i = 0; i < next; i++)
        p->headers[header_stack_elements[h][i + 1]].pointer = p->headers[header_stack_elements[h][i]].pointer;
    p->headers[header_stack_elements[h][0]].pointer =
            odp_packet_push_head(*p->wrapper, p->headers[header_stack_elements[h][0]].length);
}

void
pop(packet_descriptor_t* p, header_stack_t h) {
    uint8_t last = 0;
    while (last < header_stack_size[h] && p->headers[header_stack_elements[h][last]].pointer != NULL) last++;
    if (last > 0) {
        last--;
        debug("popping last %d\n", last);
        int i;
        for (i = 0; i < last; i++)
            p->headers[header_stack_elements[h][i]].pointer = p->headers[header_stack_elements[h][i + 1]].pointer;
        p->headers[header_stack_elements[h][last]].pointer = NULL;
    } else debug("popping from empty header stack...\n");
}

void
add_header(packet_descriptor_t* p, header_reference_t h) {
    if (p->headers[h.header_instance].pointer == NULL)
        p->headers[h.header_instance].pointer = odp_packet_push_head(*p->wrapper, h.bytewidth);
    else
        debug("Cannot add a header instance already present in the packet\n");
}

void
remove_header(packet_descriptor_t* p, header_reference_t h) {
    if (p->headers[h.header_instance].pointer != NULL) {
        p->headers[h.header_instance].pointer = NULL;
    } else {
        debug("Cannot remove a header instance not present in the packet\n");
    }
}

void
generate_digest(backend bg, char* name, int receiver, struct type_field_list* digest_field_list) {
    /*digest d = create_digest(bg, name);
    int i;
    for (i = 0; i < digest_field_list->fields_quantity; i++)
        d = add_digest_field(d, digest_field_list->field_offsets[i], digest_field_list->field_widths[i]);
    send_digest(bg, d, receiver);*/
}

void no_op(void) {
   debug("No Op");
}

void drop(packet_descriptor_t* p) {
   debug("Drop");
}

