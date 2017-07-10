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

#define BASE_PORT 10000

controller c;

uint8_t ips[3][4] = {{10,0,0,2},
                     {10,0,0,3},
                     {10,0,0,4}};

uint32_t learn_counter = 0;

void fill_own_pool_table(uint8_t ip[4]) {
	char buffer[2048]; /* TODO: ugly */
	struct p4_header* h;
	struct p4_add_table_entry* te;
	struct p4_action* a;
	struct p4_field_match_exact* exact;

	h = create_p4_header(buffer, 0, 2048);
	te = create_p4_add_table_entry(buffer,0,2048);
	strcpy(te->table_name, "own_pool");

	exact = add_p4_field_match_exact(te, 2048);
	strcpy(exact->header.name, "ipv4.dstAddr");
	memcpy(exact->bitmap, ip, 4);
	exact->length = 4*8+0;
	    
	a = add_p4_action(h, 2048);
	strcpy(a->description.name, "_no_op");

	netconv_p4_header(h);
	netconv_p4_add_table_entry(te);
	netconv_p4_field_match_exact(exact);
	netconv_p4_action(a);
	
	send_p4_msg(c, buffer, 2048);
}

void set_default_action_own_pool()
{
	char buffer[2048];
	struct p4_header* h;
	struct p4_set_default_action* sda;
	struct p4_action* a;

	h = create_p4_header(buffer, 0, sizeof(buffer));

	sda = create_p4_set_default_action(buffer,0,sizeof(buffer));
	strcpy(sda->table_name, "own_pool");

	a = &(sda->action);
	strcpy(a->description.name, "_no_op");

	netconv_p4_header(h);
	netconv_p4_set_default_action(sda);
	netconv_p4_action(a);

	send_p4_msg(c, buffer, sizeof(buffer));
}

void fill_nat_ul_tcp_table(uint8_t srcAddr[4], uint16_t srcPort, uint8_t ipAddr[4], uint16_t tcpPort)
{
    char buffer[2048]; /* TODO: ugly */
    struct p4_header* h;
    struct p4_add_table_entry* te;
    struct p4_action* a;
    struct p4_action_parameter* ap;
    struct p4_action_parameter* ap2;
    struct p4_field_match_exact* exact1;
    struct p4_field_match_exact* exact2;

    h = create_p4_header(buffer, 0, 2048);
    te = create_p4_add_table_entry(buffer,0,2048);
    strcpy(te->table_name, "nat_ul_tcp");

    exact1 = add_p4_field_match_exact(te, 2048);
    strcpy(exact1->header.name, "ipv4.srcAddr");
    memcpy(exact1->bitmap, srcAddr, 4);
    exact1->length = 4*8+0;

    exact2 = add_p4_field_match_exact(te, 2048);
    strcpy(exact2->header.name, "tcp.srcPort");
    memcpy(exact2->bitmap, &srcPort, 2);
    exact2->length = 2*8+0;

    a = add_p4_action(h, 2048);
    strcpy(a->description.name, "encodeTcp_src");

    ap = add_p4_action_parameter(h, a, 2048);	
    strcpy(ap->name, "ipAddr");
    memcpy(ap->bitmap, ipAddr, 4);
    ap->length = 4*8+0;
    ap2 = add_p4_action_parameter(h, a, 2048);	
    strcpy(ap2->name, "tcpPort");
    memcpy(ap2->bitmap, &tcpPort, 2);
    ap2->length = 2*8+0;

    netconv_p4_header(h);
    netconv_p4_add_table_entry(te);
    netconv_p4_field_match_exact(exact1);
    netconv_p4_field_match_exact(exact2);
    netconv_p4_action(a);
    netconv_p4_action_parameter(ap);
    netconv_p4_action_parameter(ap2);
    
    send_p4_msg(c, buffer, 2048);
}

void set_default_action_nat_ul_tcp()
{
	char buffer[2048];
	struct p4_header* h;
	struct p4_set_default_action* sda;
	struct p4_action* a;

	h = create_p4_header(buffer, 0, sizeof(buffer));

	sda = create_p4_set_default_action(buffer,0,sizeof(buffer));
	strcpy(sda->table_name, "nat_ul_tcp");

	a = &(sda->action);
	strcpy(a->description.name, "natTcp_learn");

	netconv_p4_header(h);
	netconv_p4_set_default_action(sda);
	netconv_p4_action(a);

	send_p4_msg(c, buffer, sizeof(buffer));
}

void fill_nat_dl_tcp_table(uint8_t dstAddr[4], uint16_t dstPort, uint8_t ipAddr[4], uint16_t tcpPort)
{
    char buffer[2048]; /* TODO: ugly */
    struct p4_header* h;
    struct p4_add_table_entry* te;
    struct p4_action* a;
    struct p4_action* a2;
    struct p4_action_parameter* ap;
    struct p4_action_parameter* ap2;
    struct p4_field_match_exact* exact1;
    struct p4_field_match_exact* exact2;

    h = create_p4_header(buffer, 0, 2048);
    te = create_p4_add_table_entry(buffer,0,2048);
    strcpy(te->table_name, "nat_dl_tcp");

    exact1 = add_p4_field_match_exact(te, 2048);
    strcpy(exact1->header.name, "ipv4.dstAddr");
    memcpy(exact1->bitmap, dstAddr, 4);
    exact1->length = 4*8+0;

    exact2 = add_p4_field_match_exact(te, 2048);
    strcpy(exact2->header.name, "tcp.dstPort");
    memcpy(exact2->bitmap, &dstPort, 2);
    exact2->length = 2*8+0;

    a = add_p4_action(h, 2048);
    strcpy(a->description.name, "resolve_dstTcp");

    ap = add_p4_action_parameter(h, a, 2048);	
    strcpy(ap->name, "ipAddr");
    memcpy(ap->bitmap, ipAddr, 4);
    ap->length = 4*8+0;
    ap2 = add_p4_action_parameter(h, a, 2048);	
    strcpy(ap2->name, "tcpPort");
    memcpy(ap2->bitmap, &tcpPort, 2);
    ap2->length = 2*8+0;

    a2 = add_p4_action(h, 2048);
    strcpy(a2->description.name, "_drop");

    netconv_p4_header(h);
    netconv_p4_add_table_entry(te);
    netconv_p4_field_match_exact(exact1);
    netconv_p4_field_match_exact(exact2);
    netconv_p4_action(a);
    netconv_p4_action_parameter(ap);
    netconv_p4_action_parameter(ap2);
    netconv_p4_action(a2);
    
    send_p4_msg(c, buffer, 2048);
}

void set_default_action_nat_dl_tcp()
{
	char buffer[2048];
	struct p4_header* h;
	struct p4_set_default_action* sda;
	struct p4_action* a;

	h = create_p4_header(buffer, 0, sizeof(buffer));

	sda = create_p4_set_default_action(buffer,0,sizeof(buffer));
	strcpy(sda->table_name, "nat_dl_tcp");

	a = &(sda->action);
	strcpy(a->description.name, "_drop");

	netconv_p4_header(h);
	netconv_p4_set_default_action(sda);
	netconv_p4_action(a);

	send_p4_msg(c, buffer, sizeof(buffer));
}

void natTcp_learn_digest(void* b) {
    uint8_t srcAddr[4];
    uint16_t srcPort[1];
    uint16_t offset=0;
    offset = sizeof(struct p4_digest);
    struct p4_digest_field* df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
    memcpy(srcAddr, df->value, 4);
    offset += sizeof(struct p4_digest_field);
    df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
    memcpy(srcPort, df->value, 2);

    printf("Ctrl: natTcp_learn_digest PORT: %d IP: %d.%d.%d.%d\n", srcPort[0], srcAddr[0],srcAddr[1],srcAddr[2],srcAddr[3]);

    uint16_t learnt_port = (BASE_PORT + learn_counter%(sizeof(ips)/4));
    uint8_t* learnt_ip = ips[learn_counter%(sizeof(ips)/4)];
    fill_nat_ul_tcp_table(srcAddr, srcPort[0], learnt_ip, learnt_port);
    fill_nat_dl_tcp_table(learnt_ip, learnt_port, srcAddr, srcPort[0]);

    printf("Ctrl: natTcp_learnt PORT: %d IP: %d.%d.%d.%d\n", learnt_port, learnt_ip[0], learnt_ip[1], learnt_ip[2], learnt_ip[3]);

    learn_counter++;
}

void fill_nat_ul_udp_table(uint8_t srcAddr[4], uint16_t srcPort, uint8_t ipAddr[4], uint16_t tcpPort)
{
    char buffer[2048]; /* TODO: ugly */
    struct p4_header* h;
    struct p4_add_table_entry* te;
    struct p4_action* a;
    struct p4_action_parameter* ap;
    struct p4_action_parameter* ap2;
    struct p4_field_match_exact* exact1;
    struct p4_field_match_exact* exact2;

    h = create_p4_header(buffer, 0, 2048);
    te = create_p4_add_table_entry(buffer,0,2048);
    strcpy(te->table_name, "nat_ul_udp");

    exact1 = add_p4_field_match_exact(te, 2048);
    strcpy(exact1->header.name, "ipv4.srcAddr");
    memcpy(exact1->bitmap, srcAddr, 4);
    exact1->length = 4*8+0;

    exact2 = add_p4_field_match_exact(te, 2048);
    strcpy(exact2->header.name, "tcp.srcPort");
    memcpy(exact2->bitmap, &srcPort, 2);
    exact2->length = 2*8+0;

    a = add_p4_action(h, 2048);
    strcpy(a->description.name, "encodeUdp_src");

    ap = add_p4_action_parameter(h, a, 2048);	
    strcpy(ap->name, "ipAddr");
    memcpy(ap->bitmap, ipAddr, 4);
    ap->length = 4*8+0;
    ap2 = add_p4_action_parameter(h, a, 2048);	
    strcpy(ap2->name, "tcpPort");
    memcpy(ap2->bitmap, &tcpPort, 2);
    ap2->length = 2*8+0;

    netconv_p4_header(h);
    netconv_p4_add_table_entry(te);
    netconv_p4_field_match_exact(exact1);
    netconv_p4_field_match_exact(exact2);
    netconv_p4_action(a);
    netconv_p4_action_parameter(ap);
    netconv_p4_action_parameter(ap2);
    
    send_p4_msg(c, buffer, 2048);
}

void set_default_action_nat_ul_udp()
{
	char buffer[2048];
	struct p4_header* h;
	struct p4_set_default_action* sda;
	struct p4_action* a;

	h = create_p4_header(buffer, 0, sizeof(buffer));

	sda = create_p4_set_default_action(buffer,0,sizeof(buffer));
	strcpy(sda->table_name, "nat_ul_udp");

	a = &(sda->action);
	strcpy(a->description.name, "natUdp_learn");

	netconv_p4_header(h);
	netconv_p4_set_default_action(sda);
	netconv_p4_action(a);

	send_p4_msg(c, buffer, sizeof(buffer));
}

void fill_nat_dl_udp_table(uint8_t dstAddr[4], uint16_t dstPort, uint8_t ipAddr[4], uint16_t tcpPort)
{
    char buffer[2048]; /* TODO: ugly */
    struct p4_header* h;
    struct p4_add_table_entry* te;
    struct p4_action* a;
    struct p4_action* a2;
    struct p4_action_parameter* ap;
    struct p4_action_parameter* ap2;
    struct p4_field_match_exact* exact1;
    struct p4_field_match_exact* exact2;

    h = create_p4_header(buffer, 0, 2048);
    te = create_p4_add_table_entry(buffer,0,2048);
    strcpy(te->table_name, "nat_dl_udp");

    exact1 = add_p4_field_match_exact(te, 2048);
    strcpy(exact1->header.name, "ipv4.dstAddr");
    memcpy(exact1->bitmap, dstAddr, 4);
    exact1->length = 4*8+0;

    exact2 = add_p4_field_match_exact(te, 2048);
    strcpy(exact2->header.name, "tcp.dstPort");
    memcpy(exact2->bitmap, &dstPort, 2);
    exact2->length = 2*8+0;

    a = add_p4_action(h, 2048);
    strcpy(a->description.name, "resolve_dstUdp");

    ap = add_p4_action_parameter(h, a, 2048);	
    strcpy(ap->name, "ipAddr");
    memcpy(ap->bitmap, ipAddr, 4);
    ap->length = 4*8+0;
    ap2 = add_p4_action_parameter(h, a, 2048);	
    strcpy(ap2->name, "tcpPort");
    memcpy(ap2->bitmap, &tcpPort, 2);
    ap2->length = 2*8+0;

    a2 = add_p4_action(h, 2048);
    strcpy(a2->description.name, "_drop");

    netconv_p4_header(h);
    netconv_p4_add_table_entry(te);
    netconv_p4_field_match_exact(exact1);
    netconv_p4_field_match_exact(exact2);
    netconv_p4_action(a);
    netconv_p4_action_parameter(ap);
    netconv_p4_action_parameter(ap2);
    netconv_p4_action(a2);
    
    send_p4_msg(c, buffer, 2048);
}

void set_default_action_nat_dl_udp()
{
	char buffer[2048];
	struct p4_header* h;
	struct p4_set_default_action* sda;
	struct p4_action* a;

	h = create_p4_header(buffer, 0, sizeof(buffer));

	sda = create_p4_set_default_action(buffer,0,sizeof(buffer));
	strcpy(sda->table_name, "nat_dl_udp");

	a = &(sda->action);
	strcpy(a->description.name, "_drop");

	netconv_p4_header(h);
	netconv_p4_set_default_action(sda);
	netconv_p4_action(a);

	send_p4_msg(c, buffer, sizeof(buffer));
}

void natUdp_learn_digest(void* b) {
    uint8_t srcAddr[4];
    uint16_t srcPort[1];
    uint16_t offset=0;
    offset = sizeof(struct p4_digest);
    struct p4_digest_field* df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
    memcpy(srcAddr, df->value, 4);
    offset += sizeof(struct p4_digest_field);
    df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
    memcpy(srcPort, df->value, 2);

    printf("Ctrl: natUdp_learn_digest PORT: %d IP: %d.%d.%d.%d\n", srcPort[0], srcAddr[0],srcAddr[1],srcAddr[2],srcAddr[3]);

    uint16_t learnt_port = (BASE_PORT + learn_counter%(sizeof(ips)/4));
    uint8_t* learnt_ip = ips[learn_counter%(sizeof(ips)/4)];
    fill_nat_ul_udp_table(srcAddr, srcPort[0], learnt_ip, learnt_port);
    fill_nat_dl_udp_table(learnt_ip, learnt_port, srcAddr, srcPort[0]);

    printf("Ctrl: natUdp_learnt PORT: %d IP: %d.%d.%d.%d\n", learnt_port, learnt_ip[0], learnt_ip[1], learnt_ip[2], learnt_ip[3]);

    learn_counter++;
}

void dhf(void* b) {
    struct p4_header* h = netconv_p4_header(unpack_p4_header(b, 0));
    if (h->type != P4T_DIGEST) {
        printf("Method is not implemented\n");
        return;
    }

    struct p4_digest* d = unpack_p4_digest(b,0);
    if (strcmp(d->field_list_name, "natTcp_learn_digest")==0) {
        printf("TCP digest received\n");
        natTcp_learn_digest(b);
    } else if (strcmp(d->field_list_name, "natUdp_learn_digest")==0) {
        printf("UDP digest received\n");
        natUdp_learn_digest(b);
    } else {
        printf("Unknown digest received: X%sX\n", d->field_list_name);
    }
}

void init_simple() {
    set_default_action_own_pool();
    for(int i=0; i<sizeof(ips)/4; i++) {
    	fill_own_pool_table(ips[i]);
    }
    set_default_action_nat_ul_tcp();
    set_default_action_nat_dl_tcp();
    set_default_action_nat_ul_udp();
    set_default_action_nat_dl_udp();
}

int main(int argc, char* argv[]) 
{
	printf("Create and configure controller...\n");

	c = create_controller_with_init(11111, 3, dhf, init_simple);

	printf("Launching controller's main loop...\n");
	execute_controller(c);

	printf("Destroy controller\n");
	destroy_controller(c);

	return 0;
}

