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


controller c;

void fill_smac_table(uint8_t port, uint8_t mac[6])
{
	char buffer[2048]; /* TODO: ugly */
	struct p4_header* h;
	struct p4_add_table_entry* te;
	struct p4_action* a;
/*	struct p4_action_parameter* ap;
	struct p4_field_match_header* fmh;*/
	struct p4_field_match_exact* exact;

	h = create_p4_header(buffer, 0, 2048);
	te = create_p4_add_table_entry(buffer,0,2048);
	strcpy(te->table_name, "smac");

	exact = add_p4_field_match_exact(te, 2048);
	strcpy(exact->header.name, "ethernet.srcAddr");
	memcpy(exact->bitmap, mac, 6);
	exact->length = 6*8+0;
	
	a = add_p4_action(h, 2048);
	strcpy(a->description.name, "_nop");

	netconv_p4_header(h);
	netconv_p4_add_table_entry(te);
	netconv_p4_field_match_exact(exact);
	netconv_p4_action(a);

	send_p4_msg(c, buffer, 2048);
}

void fill_dmac_table(uint8_t port, uint8_t mac[6])
{
        char buffer[2048]; /* TODO: ugly */
        struct p4_header* h;
        struct p4_add_table_entry* te;
        struct p4_action* a;
        struct p4_action_parameter* ap;
        struct p4_field_match_exact* exact;

        h = create_p4_header(buffer, 0, 2048);
        te = create_p4_add_table_entry(buffer,0,2048);
	strcpy(te->table_name, "dmac");

        exact = add_p4_field_match_exact(te, 2048);
        strcpy(exact->header.name, "ethernet.dstAddr");
        memcpy(exact->bitmap, mac, 6);
        exact->length = 6*8+0;

        a = add_p4_action(h, 2048);
        strcpy(a->description.name, "forward");
	
	ap = add_p4_action_parameter(h, a, 2048);	
	strcpy(ap->name, "port");
	memcpy(ap->bitmap, &port, 1);
	ap->length = 1*8+0;

        netconv_p4_header(h);
        netconv_p4_add_table_entry(te);
        netconv_p4_field_match_exact(exact);
        netconv_p4_action(a);
	netconv_p4_action_parameter(ap);

        send_p4_msg(c, buffer, 2048);
}

void mac_learn_digest(void* b) {
    uint8_t mac[6];
    uint8_t port[2];
    uint16_t offset=0;
    offset = sizeof(struct p4_digest);
    struct p4_digest_field* df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
    memcpy(mac, df->value, 6);
    offset += sizeof(struct p4_digest_field);
    df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
    memcpy(port, df->value, 2);

    uint8_t p = port[0];
    printf("Ctrl: mac_learn_digest PORT: %d MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", p, mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    fill_dmac_table(p, mac);
    fill_smac_table(p, mac);
}

void test_learn_ip(void* b) {
    uint8_t ip[4];
    uint16_t pr;
    int i1;
    int i2;
    int i3;

    uint16_t offset=0;
    offset = sizeof(struct p4_digest);
    struct p4_digest_field* df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
    memcpy(ip, df->value, 4);
    offset += sizeof(struct p4_digest_field);
    df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
    memcpy(&pr, df->value, 2);
    offset += sizeof(struct p4_digest_field);
    df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
    memcpy(&i1, df->value, 4);
    offset += sizeof(struct p4_digest_field);
    df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
    memcpy(&i2, df->value, 4);
    offset += sizeof(struct p4_digest_field);
    df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
    memcpy(&i3, df->value, 4);

    char buffer[2048]; /* TODO: ugly */
    struct p4_header* h;
    struct p4_add_table_entry* te;
    struct p4_action* a;
    struct p4_action_parameter *ap1, *ap2, *ap3;
    struct p4_field_match_lpm* lpm;
    h = create_p4_header(buffer, 0, 2048);
    te = create_p4_add_table_entry(buffer,0,2048);
    strcpy(te->table_name, "table1");
    lpm = add_p4_field_match_lpm(te, 2048);
    strcpy(lpm->header.name, "field1");
    memcpy(lpm->bitmap, ip, 4);
    lpm->prefix_length = pr;
    a = add_p4_action(h, 2048);
    strcpy(a->description.name, "korte");
    ap1 = add_p4_action_parameter(h, a, 2048);
    memcpy(ap1->bitmap, &i1, 4);
    ap2 = add_p4_action_parameter(h, a, 2048);
    memcpy(ap2->bitmap, &i2, 4);
    ap3 = add_p4_action_parameter(h, a, 2048);
    memcpy(ap3->bitmap, &i3, 4);
    netconv_p4_header(h);
    netconv_p4_add_table_entry(te);
    netconv_p4_field_match_lpm(lpm);
    netconv_p4_action(a);
    netconv_p4_action_parameter(ap1);
    netconv_p4_action_parameter(ap2);
    netconv_p4_action_parameter(ap3);
    send_p4_msg(c, buffer, 2048);
}

void dhf(void* b) {
    struct p4_header* h = netconv_p4_header(unpack_p4_header(b, 0));
    if (h->type != P4T_DIGEST) {
        printf("Method is not implemented\n");
        return;
    }

    struct p4_digest* d = unpack_p4_digest(b,0);
    if (strcmp(d->field_list_name, "mac_learn_digest")==0) {
        mac_learn_digest(b);
    } else if (strcmp(d->field_list_name, "test_learn_ip")==0) {
        test_learn_ip(b);
    } else {
        printf("Unknown digest received: X%sX\n", d->field_list_name);
    }
}

void set_default_action_smac()
{
	char buffer[2048]; /* TODO: ugly */
	struct p4_header* h;
	struct p4_set_default_action* sda;
	struct p4_action* a;

	printf("Generate set_default_action message for table smac\n");
	
	h = create_p4_header(buffer, 0, sizeof(buffer));

	sda = create_p4_set_default_action(buffer,0,sizeof(buffer));
	strcpy(sda->table_name, "smac");

	a = &(sda->action);
	strcpy(a->description.name, "mac_learn");

	netconv_p4_header(h);
	netconv_p4_set_default_action(sda);
	netconv_p4_action(a);

	send_p4_msg(c, buffer, sizeof(buffer));
}

void set_default_action_dmac()
{
	char buffer[2048]; /* TODO: ugly */
	struct p4_header* h;
	struct p4_set_default_action* sda;
	struct p4_action* a;

	printf("Generate set_default_action message for table dmac\n");
	
	h = create_p4_header(buffer, 0, sizeof(buffer));

	sda = create_p4_set_default_action(buffer,0,sizeof(buffer));
	strcpy(sda->table_name, "dmac");

	a = &(sda->action);
	strcpy(a->description.name, "bcast");

	netconv_p4_header(h);
	netconv_p4_set_default_action(sda);
	netconv_p4_action(a);

	send_p4_msg(c, buffer, sizeof(buffer));
}



void init() {
	set_default_action_smac();
	set_default_action_dmac();
}

int main() 
{
	printf("Create and configure controller...\n");
	c = create_controller_with_init(11111, 3, dhf, init);

	printf("Launching controller's main loop...\n");
	execute_controller(c);

	printf("Destroy controller\n");
	destroy_controller(c);

	return 0;
}

