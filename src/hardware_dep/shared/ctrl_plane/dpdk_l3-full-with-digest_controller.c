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
#include "controller.h"
#include "messages.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#define MAX_IPS 60000

controller c;

void fill_ipv4_lpm_table(uint8_t ip[4], uint32_t nhgrp)
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
    strcpy(te->table_name, "ipv4_lpm");

    lpm = add_p4_field_match_lpm(te, 2048);
    strcpy(lpm->header.name, "ipv4.dstAddr");
    memcpy(lpm->bitmap, ip, 4);
    lpm->prefix_length = 16;

    a = add_p4_action(h, 2048);
    strcpy(a->description.name, "set_nhop");

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

    h = create_p4_header(buffer, 0, 2048);
    te = create_p4_add_table_entry(buffer,0,2048);
    strcpy(te->table_name, "nexthops");

    exact = add_p4_field_match_exact(te, 2048);
    strcpy(exact->header.name, "routing_metadata.nhgroup");
    memcpy(exact->bitmap, &nhgroup, 4);
    exact->length = 4*8+0;

    a = add_p4_action(h, 2048);
    strcpy(a->description.name, "forward");

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
}

void set_default_action_ipv4_lpm()
{
	char buffer[2048]; /* TODO: ugly */
	struct p4_header* h;
	struct p4_set_default_action* sda;
	struct p4_action* a;

	printf("Generate set_default_action message for table ipv4_lpm\n");
	
	h = create_p4_header(buffer, 0, sizeof(buffer));

	sda = create_p4_set_default_action(buffer,0,sizeof(buffer));
	strcpy(sda->table_name, "ipv4_lpm");

	a = &(sda->action);
	strcpy(a->description.name, "miss");

	netconv_p4_header(h);
	netconv_p4_set_default_action(sda);
	netconv_p4_action(a);

	send_p4_msg(c, buffer, sizeof(buffer));
}

void dhf(void* b) {
    struct p4_header* h = netconv_p4_header(unpack_p4_header(b, 0));
    if (h->type != P4T_DIGEST) {
        printf("Method is not implemented\n");
        return;
    }

    struct p4_digest* d = unpack_p4_digest(b,0);
    if (strcmp(d->field_list_name, "miss_on_iplookup_digest")==0) {
        miss_on_iplookup_digest(b);
    } else {
        printf("Unknown digest received: X%sX\n", d->field_list_name);
    }
}

void miss_on_iplookup_digest(void* b) {
    uint8_t ipv4addr[48];
    uint8_t port[2];
    offset = sizeof(struct p4_digest);
    struct p4_digest_field* df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
    memcpy(ipvaddr, df->value, 48);
    offset += sizeof(struct p4_digest_field);
    df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
    memcpy(port, df->value, 2);

    printf("Ctrl: miss_on_iplookup_digest - TODO, nothing happened");
}

void init_simple() {
	uint8_t ip[4] = {10,0,99,99};
	uint8_t mac[6] = {0xd2, 0x69, 0x0f, 0xa8, 0x39, 0x9c};
    uint8_t smac[6] = {0xd2, 0x69, 0x0f, 0x00, 0x00, 0x9c};
	uint8_t port = 15;
    uint32_t nhgrp = 0;

	fill_ipv4_lpm_table(ip, nhgrp);
    fill_nexthops_table(nhgrp, port, smac, mac);
    set_default_action_ipv4_lpm();
}

uint32_t ip_array[MAX_IPS];
int ip_count = -1;

int read_ip_prefixes_from_file(char *filename) {
	FILE *f;
	char line[100];

	f = fopen(filename, "r");
	if (f == NULL) return -1;

	while (fgets(line, sizeof(line), f)) {
		line[strlen(line)-1] = '\0';
		if (inet_pton(AF_INET, line, &(ip_array[++ip_count])) != 1) { /*TODO: check if it is in big-endian or not!!!*/
			printf("%s is not an ip address.\n",line);
			fclose(f);
			return -1;
		}
	}

	fclose(f);
	return 0;
}


void init_complex() {
	uint8_t mac[6] = {0xd2, 0x69, 0x0f, 0xa8, 0x39, 0x9c};
    uint8_t smac[6] = {0xd2, 0x69, 0x0f, 0x00, 0x00, 0x9c};
	uint8_t port = 1;
    uint32_t nhgrp = 0;
	int i=0;

	for (;i<=ip_count;++i) {
		/* Delegate routing table entries */
		fill_ipv4_lpm_table((uint8_t*)&(ip_array[i]), nhgrp);
	}
    
    fill_nexthops_table(nhgrp, port, smac, mac);
    
    set_default_action_ipv4_lpm();
}


int main(int argc, char* argv[]) 
{
	printf("Create and configure controller...\n");

	if (argc>1) {
		if (argc!=2) {
			printf("Too many arguments...\nUsage: %s <filename(optional)>\n", argv[0]);
			return -1;
		}
		printf("Command line argument is present...\nLoading configuration data...\n");
		if (read_ip_prefixes_from_file(argv[1])<0) {
			printf("File cannnot be opened...\n");
			return -1;
		}
		c = create_controller_with_init(11111, 3, dhf, init_complex);
	}
	else {
		c = create_controller_with_init(11111, 3, dhf, init_simple);
	}

	printf("Launching controller's main loop...\n");
	execute_controller(c);

	printf("Destroy controller\n");
	destroy_controller(c);

	return 0;
}

