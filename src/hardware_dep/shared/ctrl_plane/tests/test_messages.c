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
#include "messages.h"
#include "handlers.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define BUFFLEN 1024

void test_p4_header()
{
        char buffer[BUFFLEN];
        struct p4_header* header;
        struct p4_header* header2;

        header = create_p4_header(buffer,0, BUFFLEN);
        header->xid = 112222;
        header->type = P4T_ECHO_REQUEST;

        header2 = unpack_p4_header(buffer, 0);

        check_p4_header( header, header2 );
}

void test_p4_set_default_action()
{
	char buffer[BUFFLEN];
        struct p4_header* h;
        struct p4_header* h2; 
	struct p4_set_default_action* sda;
	struct p4_set_default_action* sda2;
	struct p4_action* a;
	struct p4_action* a2;
	struct p4_action_parameter* ap;
	struct p4_action_parameter* ap2;
	uint32_t val = 9987112;
	uint16_t offset = 0;
	uint16_t i = 0;
	struct p4_ctrl_msg ctrl_m;

	h = create_p4_header(buffer,0, BUFFLEN);
	h->xid = 112223;

	sda = create_p4_set_default_action(buffer,0,BUFFLEN);
	strcpy(sda->table_name,"smac");

	a = &(sda->action);	
	strcpy(a->description.name,"smac_hit");

	ap = add_p4_action_parameter(h, a, BUFFLEN);
	strcpy(ap->name, "port");
	memcpy(ap->bitmap, &val, sizeof(val));
	ap->length = sizeof(val)*8; /* length of a bitvector */
	
	assert(h->length == (sizeof(struct p4_set_default_action) + sizeof(struct p4_action_parameter)));
	assert(a->param_size == 1);

	h2 = unpack_p4_header(buffer, 0);

	assert(h2->xid == 112223);

	sda2 = unpack_p4_set_default_action(buffer, 0);
	a2 = &(sda2->action);
	offset = sizeof(struct p4_set_default_action);

	assert( a2->param_size == 1);

	for (i = 0;i < a2->param_size; ++i) {
		ap2 = unpack_p4_action_parameter(buffer,offset);
		offset += sizeof(struct p4_action_parameter);
	}

	assert(ap2->length == sizeof(uint32_t)*8);
	assert(*(uint32_t*)&(ap2->bitmap) == val); 

	/* Testing the handler */

	assert(handle_p4_set_default_action(sda2, &ctrl_m) == 0);

	assert(ctrl_m.num_action_params == 1);
	assert(ctrl_m.action_params[0]->length == sizeof(uint32_t)*8);
	assert(*(uint32_t*)&(ctrl_m.action_params[0]->bitmap) == val);

}


int main()
{
	printf("Test cases:\n");
	printf("* test_p4_header");
	fflush(stdout);
	test_p4_header();
	printf(" OK\n");


	printf("* test_p4_set_default_action");
        fflush(stdout);
	test_p4_set_default_action();
        printf(" OK\n");

	return 0;
}
