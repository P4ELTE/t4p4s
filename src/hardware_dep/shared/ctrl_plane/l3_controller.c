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

void fill_ipv4_fib_lpm_table(uint8_t ip[4], uint8_t port, uint8_t mac[6])
{
        char buffer[2048]; /* TODO: ugly */
        struct p4_header* h;
        struct p4_add_table_entry* te;
        struct p4_action* a;
        struct p4_action_parameter* ap,* ap2;
        struct p4_field_match_exact* exact; // TODO: replace to lpm

        h = create_p4_header(buffer, 0, 2048);
        te = create_p4_add_table_entry(buffer,0,2048);
	strcpy(te->table_name, "ipv4_fib_lpm");

        exact = add_p4_field_match_exact(te, 2048);
        strcpy(exact->header.name, "ipv4.dstAddr");
        memcpy(exact->bitmap, ip, 4);
        exact->length = 4*8+0;

        a = add_p4_action(h, 2048);
        strcpy(a->description.name, "fib_hit_nexthop");

/*        ap2 = add_p4_action_parameter(h, a, 2048);
        strcpy(ap->name, "port");
        memcpy(ap->bitmap, &port, 1);
        ap->length = 1*8+0;
*/	
	ap = add_p4_action_parameter(h, a, 2048);	
	strcpy(ap->name, "dmac");
	memcpy(ap->bitmap, mac, 6);
	ap->length = 6*8+0;

	ap2 = add_p4_action_parameter(h, a, 2048);	
	strcpy(ap2->name, "port");
    ap2->bitmap[0] = port;
    ap2->bitmap[1] = 0;
	ap2->length = 2*8+0;

        netconv_p4_header(h);
        netconv_p4_add_table_entry(te);
        netconv_p4_field_match_exact(exact);
        netconv_p4_action(a);
	netconv_p4_action_parameter(ap);
	netconv_p4_action_parameter(ap2);

        send_p4_msg(c, buffer, 2048);
}

void mac_learn_digest(void* b) {
   
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

    printf("test_learn_ip\n");

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
       printf("Unknown digest received\n");
}

int main() 
{
	uint8_t ip[4] = {10,0,0,1};
	uint8_t mac[6] = {0xd2, 0x69, 0x0f, 0xa8, 0x39, 0x9c};
	uint8_t port = 1;

	printf("Create and configure controller...\n");
	c = create_controller(11111, 3, dhf);

	fill_ipv4_fib_lpm_table(ip, port, mac);	

	printf("Launching controller's main loop...\n");
	execute_controller(c);

	printf("Destroy controller\n");
	destroy_controller(c);

	return 0;
}

