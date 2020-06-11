// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#include "controller.h"
#include "messages.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

controller c;

extern void notify_controller_initialized();

void fill_table(uint8_t addr, uint8_t dd)
{
    char buffer[2048];
    struct p4_header* h;
    struct p4_add_table_entry* te;
    struct p4_action* a;
    // struct p4_action_parameter* ap;
    // struct p4_field_match_header* fmh;
    struct p4_field_match_exact* exact;

    h = create_p4_header(buffer, 0, 2048);
    te = create_p4_add_table_entry(buffer,0,2048);
    strcpy(te->table_name, "t_0");

    exact = add_p4_field_match_exact(te, 2048);
    strcpy(exact->header.name, "dummy.addr");
    memcpy(exact->bitmap, dd, 1);
    exact->length = 1*8+0;

    a = add_p4_action(h, 2048);
    strcpy(a->description.name, "found");

    netconv_p4_header(h);
    netconv_p4_add_table_entry(te);
    netconv_p4_field_match_exact(exact);
    netconv_p4_action(a);

    send_p4_msg(c, buffer, 2048);
}

void learn_digest(void* b) {
    uint8_t addr;
    uint8_t dd;
    uint16_t offset=0;
    offset = sizeof(struct p4_digest);
    struct p4_digest_field* df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
    memcpy(addr, df->value, 1);
    offset += sizeof(struct p4_digest_field);
    df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
    memcpy(dd, df->value, 1);

    fill_table(addr, dd);
}

void dhf(void* b) {
    struct p4_header* h = netconv_p4_header(unpack_p4_header(b, 0));
    if (h->type != P4T_DIGEST) {
        printf("Method is not implemented\n");
        return;
    }

    struct p4_digest* d = unpack_p4_digest(b,0);
    if (strcmp(d->field_list_name, "learn_digest")==0) {
        learn_digest(b);
    } else {
        printf("Unknown digest received: X%sX\n", d->field_list_name);
    }
}

void set_default_action()
{
    char buffer[2048];
    struct p4_header* h;
    struct p4_set_default_action* sda;
    struct p4_action* a;

    printf("Generate set_default_action message for table t\n");

    h = create_p4_header(buffer, 0, sizeof(buffer));

    sda = create_p4_set_default_action(buffer,0,sizeof(buffer));
    strcpy(sda->table_name, "t_0");

    a = &(sda->action);
    strcpy(a->description.name, "learn");

    netconv_p4_header(h);
    netconv_p4_set_default_action(sda);
    netconv_p4_action(a);

    send_p4_msg(c, buffer, sizeof(buffer));
}

void init() {
    printf("Set default actions.\n");
    set_default_action();
    notify_controller_initialized();
}

int main(int argc, char* argv[])
{
    printf("Create and configure controller...\n");
    c = create_controller_with_init(11111, 3, dhf, init);

    printf("Launching controller's main loop...\n");
    execute_controller(c);

    printf("Destroy controller\n");
    destroy_controller(c);

    return 0;
}

