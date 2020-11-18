// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#include "controller.h"
#include "messages.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_IPS 60000

extern int usleep(__useconds_t usec);
extern void notify_controller_initialized();

controller c;

void fill_ipv4_lpm_table(uint8_t ip[4],uint8_t prefix, uint32_t nhgrp)
{
    char buffer[2048]; /* TODO: ugly */
    struct p4_header* h;
    struct p4_add_table_entry* te;
    struct p4_action* a;
    struct p4_action_parameter* ap;
    struct p4_field_match_lpm* lpm;

	printf("ipv4_lpm\n");
    h = create_p4_header(buffer, 0, 2048);
    te = create_p4_add_table_entry(buffer,0,2048);
    strcpy(te->table_name, ".ipv4_lpm");

    lpm = add_p4_field_match_lpm(te, 2048);
    strcpy(lpm->header.name, "ipv4.dstAddr");
    memcpy(lpm->bitmap, ip, 4);
    lpm->prefix_length = prefix;

    a = add_p4_action(h, 2048);
    strcpy(a->description.name, ".set_nhop");

    printf("add nhgrp\n");
    ap = add_p4_action_parameter(h, a, 2048);	
    strcpy(ap->name, "nhgroup");
    memcpy(ap->bitmap, &nhgrp, 4);
    ap->length = 4*8+0;

    printf("NH-1\n");
    netconv_p4_header(h);
    netconv_p4_add_table_entry(te);
    netconv_p4_field_match_lpm(lpm);
    netconv_p4_action(a);
    netconv_p4_action_parameter(ap);

    send_p4_msg(c, buffer, 2048);
    usleep(1200);
}

void fill_nexthops_table(uint32_t nhgroup, uint8_t port, uint8_t smac[6], uint8_t dmac[6])
{
    char buffer[2048]; /* TODO: ugly */
    struct p4_header* h;
    struct p4_add_table_entry* te;
    struct p4_action* a;
    struct p4_action_parameter* ap, *ap2, *ap3;
    struct p4_field_match_exact* exact;

    printf("nexthops\n");
    printf("Group: %d Port: %d Smac: %d:%d:%d:%d:%d:%d Dmac: %d:%d:%d:%d:%d:%d\n",
           nhgroup, port,
           smac[0], smac[1], smac[2], smac[3], smac[4], smac[5],
           dmac[0], dmac[1], dmac[2], dmac[3], dmac[4], dmac[5]);

    h = create_p4_header(buffer, 0, 2048);
    te = create_p4_add_table_entry(buffer,0,2048);
    strcpy(te->table_name, ".nexthops");

    exact = add_p4_field_match_exact(te, 2048);
    strcpy(exact->header.name, "routing_metadata.nhgroup");
    memcpy(exact->bitmap, &nhgroup, 4);
    exact->length = 4*8+0;

    a = add_p4_action(h, 2048);
    strcpy(a->description.name, ".forward");

    ap = add_p4_action_parameter(h, a, 2048);	
    strcpy(ap->name, "dmac");
    memcpy(ap->bitmap, dmac, 6);
    ap->length = 6*8+0;
    ap2 = add_p4_action_parameter(h, a, 2048);	
    strcpy(ap2->name, "smac");
    memcpy(ap2->bitmap, smac, 6);
    ap2->length = 6*8+0;

    ap3 = add_p4_action_parameter(h, a, 2048);	
    strcpy(ap3->name, "port");
    ap3->bitmap[0] = port;
    ap3->bitmap[1] = 0;
    ap3->length = 2*8+0;

    netconv_p4_header(h);
    netconv_p4_add_table_entry(te);
    netconv_p4_field_match_exact(exact);
    netconv_p4_action(a);
    netconv_p4_action_parameter(ap);
    netconv_p4_action_parameter(ap2);
    netconv_p4_action_parameter(ap3);

    send_p4_msg(c, buffer, 2048);
    usleep(1200);
}

void set_default_action_nexthops()
{
    char buffer[2048];
    struct p4_header* h;
    struct p4_set_default_action* sda;
    struct p4_action* a;

    printf("Generate set_default_action message for table nexthops\n");

    h = create_p4_header(buffer, 0, sizeof(buffer));

    sda = create_p4_set_default_action(buffer,0,sizeof(buffer));
    strcpy(sda->table_name, ".nexthops");

    a = &(sda->action);
    strcpy(a->description.name, "._drop");

    netconv_p4_header(h);
    netconv_p4_set_default_action(sda);
    netconv_p4_action(a);

    send_p4_msg(c, buffer, sizeof(buffer));
}

void set_default_action_ipv4_lpm()
{
    char buffer[2048];
    struct p4_header* h;
    struct p4_set_default_action* sda;
    struct p4_action* a;

    printf("Generate set_default_action message for table ipv4_lpm\n");

    h = create_p4_header(buffer, 0, sizeof(buffer));

    sda = create_p4_set_default_action(buffer,0,sizeof(buffer));
    strcpy(sda->table_name, ".ipv4_lpm");

    a = &(sda->action);
    strcpy(a->description.name, "._drop");

    netconv_p4_header(h);
    netconv_p4_set_default_action(sda);
    netconv_p4_action(a);

    send_p4_msg(c, buffer, sizeof(buffer));
}

void dhf(void* b) {
       printf("Unknown digest received\n");
}

void init_simple() {
	uint8_t ip[4] = {10,0,99,99};
	uint8_t mac[6] = {0xd2, 0x69, 0x0f, 0xa8, 0x39, 0x9c};
	uint8_t smac[6] = {0xd2, 0x69, 0x0f, 0x00, 0x00, 0x9c};
	uint8_t port = 15;
	uint32_t nhgrp = 0;

    set_default_action_nexthops();
    set_default_action_ipv4_lpm();

	fill_ipv4_lpm_table(ip, 16, nhgrp);
	fill_nexthops_table(nhgrp, port, smac, mac);
}

int main(int argc, char* argv[]) 
{
	printf("Create and configure controller...\n");

    c = create_controller_with_init(11111, 3, dhf, init_simple);

    notify_controller_initialized();

	printf("Launching controller's main loop...\n");
	execute_controller(c);

	printf("Destroy controller\n");
	destroy_controller(c);

	return 0;
}

