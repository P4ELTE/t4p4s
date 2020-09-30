// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Eotvos Lorand University, Budapest, Hungary

#include "controller.h"
#include "messages.h"
#include "dpdk_controller_dictionary.h"
#include "dpdk_ctrl_common.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>

extern controller c;

// ------------------

char buffer[2048];


int send_exact_entry(uint8_t port, uint8_t mac[6], const char* table_name, const char* header_name, const char* action_name, const char* par1, const char* par2)
{
    struct p4_action_parameter* ap1;
    struct p4_action_parameter* ap2;

    struct p4_header* h = create_p4_header(buffer, 0, 2048);
    struct p4_add_table_entry* te = create_p4_add_table_entry(buffer,0,2048);
    strcpy(te->table_name, translate(table_name));

    struct p4_field_match_exact* exact = add_p4_field_match_exact(te, 2048);
    strcpy(exact->header.name, translate(header_name));
    memcpy(exact->bitmap, mac, 6);
    exact->length = 6*8+0;

    struct p4_action* a = add_p4_action(h, 2048);
    strcpy(a->description.name, translate(action_name));

    if (par1 != 0) {
        ap1 = add_p4_action_parameter(h, a, 2048);
        strcpy(ap1->name, translate(par1));
        memcpy(ap1->bitmap, &port, 1);
        ap1->length = 1*8+0;
    }

    if (par2 != 0) {
        ap2 = add_p4_action_parameter(h, a, 2048);
        strcpy(ap2->name, translate(par2));
        memcpy(ap2->bitmap, &port, 1);
        ap2->length = 1*8+0;
    }

    netconv_p4_header(h);
    netconv_p4_add_table_entry(te);
    netconv_p4_field_match_exact(exact);
    netconv_p4_action(a);
    if (par1 != 0) {
        netconv_p4_action_parameter(ap1);
    }
    if (par2 != 0) {
        netconv_p4_action_parameter(ap2);
    }

    send_p4_msg(c, buffer, 2048);
    printf("<<<< EXACT %s %s.%s %hhx:%hhx:%hhx:%hhx:%hhx:%hhx %hd\n", table_name, header_name, action_name, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], port);
    return 0;
}


int send_lpm_entry(uint8_t ip[4], uint16_t prefix_length, const char* table_name, const char* header_name, const char* action_name, int i1, int i2, int i3)
{
    struct p4_header* h = create_p4_header(buffer, 0, 2048);
    struct p4_add_table_entry* te = create_p4_add_table_entry(buffer,0,2048);
    strcpy(te->table_name, table_name);

    struct p4_field_match_lpm* lpm = add_p4_field_match_lpm(te, 2048);
    strcpy(lpm->header.name, header_name);
    memcpy(lpm->bitmap, ip, 4);
    lpm->prefix_length = prefix_length;

    struct p4_action* a = add_p4_action(h, 2048);
    strcpy(a->description.name, action_name);

    struct p4_action_parameter* ap1 = add_p4_action_parameter(h, a, 2048);
    memcpy(ap1->bitmap, &i1, 4);
    struct p4_action_parameter* ap2 = add_p4_action_parameter(h, a, 2048);
    memcpy(ap2->bitmap, &i2, 4);
    struct p4_action_parameter* ap3 = add_p4_action_parameter(h, a, 2048);
    memcpy(ap3->bitmap, &i3, 4);

    netconv_p4_header(h);
    netconv_p4_add_table_entry(te);
    netconv_p4_field_match_lpm(lpm);
    netconv_p4_action(a);
    netconv_p4_action_parameter(ap1);
    netconv_p4_action_parameter(ap2);
    netconv_p4_action_parameter(ap3);

    send_p4_msg(c, buffer, 2048);
    printf("<<<< EXACT %s %d %s.%s %hhx.%hhx.%hhx.%hhx\n", table_name, prefix_length, header_name, action_name, ip[0], ip[1], ip[2], ip[3]);
    return 0;
}


int fill_teid_rate_limiter_table(uint32_t teid, const char* table_name, const char* header_name, const char* mode) {
    struct p4_header* h = create_p4_header(buffer, 0, 2048);
    struct p4_add_table_entry* te = create_p4_add_table_entry(buffer, 0, 2048);
    strcpy(te->table_name, table_name);

    struct p4_field_match_exact* exact = add_p4_field_match_exact(te, 2048);
    strcpy(exact->header.name, header_name);
    memcpy(exact->bitmap, &teid, 1);
    exact->length = 1 * 8 + 0;

    struct p4_action* a = add_p4_action(h, 2048);
    if (strcmp(mode, "APPLY_METER") && strcmp(mode, "_NOP") && strcmp(mode, "_DROP")) {
        printf("Invalid teid mode %s\n", mode);
        return -1;
    }
    strcpy(a->description.name, "apply_meter");

    netconv_p4_header(h);
    netconv_p4_add_table_entry(te);
    netconv_p4_field_match_exact(exact);
    netconv_p4_action(a);

    send_p4_msg(c, buffer, 2048);
    return 0;
}

void fill_t_fwd_table(uint16_t inport, uint16_t port, uint8_t mac[6], int wmac)
{
        char buffer[2048];
        struct p4_header* h;
        struct p4_add_table_entry* te;
        struct p4_action* a;
        struct p4_action_parameter* ap;
        struct p4_action_parameter* ap2;
        struct p4_field_match_exact* exact;

        h = create_p4_header(buffer, 0, 2048);
        te = create_p4_add_table_entry(buffer,0,2048);
        strcpy(te->table_name, "t_fwd_0");

        exact = add_p4_field_match_exact(te, 2048);
        strcpy(exact->header.name, "standard_metadata.ingress_port");
        memcpy(exact->bitmap, &inport , 2);
        exact->length = 2*8+0;

        if (wmac) {
                a = add_p4_action(h, 2048);
                strcpy(a->description.name, "forward_rewrite");
        
                ap = add_p4_action_parameter(h, a, 2048);        
                strcpy(ap->name, "port");
                memcpy(ap->bitmap, &port, 2);
                ap->length = 2*8+0;

                ap2 = add_p4_action_parameter(h, a, 2048);
                strcpy(ap2->name, "mac");
                memcpy(ap2->bitmap, mac, 6);
                ap2->length = 6*8+0;

                netconv_p4_header(h);
                netconv_p4_add_table_entry(te);
                netconv_p4_field_match_exact(exact);
                netconv_p4_action(a);
                netconv_p4_action_parameter(ap);
                netconv_p4_action_parameter(ap2);
        } else {
                a = add_p4_action(h, 2048);
                strcpy(a->description.name, "forward");
        
                ap = add_p4_action_parameter(h, a, 2048);        
                strcpy(ap->name, "port");
                memcpy(ap->bitmap, &port, 2);
                ap->length = 2*8+0;

                netconv_p4_header(h);
                netconv_p4_add_table_entry(te);
                netconv_p4_field_match_exact(exact);
                netconv_p4_action(a);
                netconv_p4_action_parameter(ap);

        }

        send_p4_msg(c, buffer, 2048);
}

// ------------------

void undigest_macport(digest_macport_t* dig, void* digest) {
    uint16_t offset = sizeof(struct p4_digest);

    struct p4_digest_field* df = netconv_p4_digest_field(unpack_p4_digest_field(digest, offset));
    memcpy(&(dig->mac), df->value, 6);
    offset += sizeof(struct p4_digest_field);

    uint8_t port[2];
    df = netconv_p4_digest_field(unpack_p4_digest_field(digest, offset));
    memcpy(port, df->value, 2);

    dig->port = port[0];

    printf(">>>> digest PORT: %d MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", dig->port, dig->mac[0],dig->mac[1],dig->mac[2],dig->mac[3],dig->mac[4],dig->mac[5]);
}

void undigest_ip(digest_ip_t* dig, void* digest) {
    uint16_t offset=0;
    offset = sizeof(struct p4_digest);
    struct p4_digest_field* df = netconv_p4_digest_field(unpack_p4_digest_field(digest, offset));

    memcpy(dig->ip, df->value, 4);
    offset += sizeof(struct p4_digest_field);

    df = netconv_p4_digest_field(unpack_p4_digest_field(digest, offset));
    memcpy(&(dig->prefix_length), df->value, 2);
    offset += sizeof(struct p4_digest_field);

    df = netconv_p4_digest_field(unpack_p4_digest_field(digest, offset));
    memcpy(&(dig->i1), df->value, 4);
    offset += sizeof(struct p4_digest_field);

    df = netconv_p4_digest_field(unpack_p4_digest_field(digest, offset));
    memcpy(&(dig->i2), df->value, 4);
    offset += sizeof(struct p4_digest_field);

    df = netconv_p4_digest_field(unpack_p4_digest_field(digest, offset));
    memcpy(&(dig->i3), df->value, 4);

    uint8_t* port_bytes = dig->ip;
    printf(">>>> digest IP: %d.%d.%d.%d PORT: %d DATA: %d %d %d\n", port_bytes[0], port_bytes[1], port_bytes[2], port_bytes[3], dig->prefix_length, dig->i1, dig->i2, dig->i3);
}

// ------------------

void notify_controller_initialized()
{
    char buffer[sizeof(struct p4_header)];
    struct p4_header* h;

    h = create_p4_header(buffer, 0, sizeof(struct p4_header));
    h->type = P4T_CTRL_INITIALIZED;

    netconv_p4_header(h);

    send_p4_msg(c, buffer, sizeof(struct p4_header));
}

void set_table_default_action(const char* table_nickname, const char* table_name, const char* default_action_name) {
    printf("Generate set_default_action message for table %s\n", table_nickname);

    struct p4_header* h = create_p4_header(buffer, 0, sizeof(buffer));

    struct p4_set_default_action* sda = create_p4_set_default_action(buffer, 0, sizeof(buffer));
    strcpy(sda->table_name, table_name);

    struct p4_action* a = & (sda->action);
    strcpy(a->description.name, default_action_name);

    netconv_p4_header(h);
    netconv_p4_set_default_action(sda);
    netconv_p4_action(a);

    send_p4_msg(c, buffer, sizeof(buffer));
}
