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
#include "ctrl_plane_backend.h"
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "handlers.h"

void cbf_test(struct p4_ctrl_msg* ctrl_m)
{
	int i;
	struct p4_field_match_header* fmh;
	struct p4_field_match_exact* exact = 0;
	printf("-----------CALLBACK---------------\n");
	
	if (ctrl_m->type == P4T_ADD_TABLE_ENTRY)
	{
		printf(" #### ADD_TABLE_ENTRY\n");
		printf("   ## Table name: %s\n", ctrl_m->table_name);
		for (i=0;i<ctrl_m->num_field_matches;++i) {
			fmh = ctrl_m->field_matches[i];
			if (fmh->type==P4_FMT_EXACT) {
				exact = (struct p4_field_match_exact*)fmh;
				printf("   ## Field match %d: %s %d bits exact %02x:%02x:%02x:%02x:%02x:%02x\n", i, fmh->name,exact->length, exact->bitmap[0],exact->bitmap[1],exact->bitmap[2],(uint8_t)exact->bitmap[3],exact->bitmap[4],exact->bitmap[5] );
			}
			else
				printf("   ## Field match %d: %s not exact\n", i, fmh->name);
		}
		printf("   ## Action name: %s\n", ctrl_m->action_name);
		for (i=0;i<ctrl_m->num_action_params;++i)
		{
			printf("   ## Action param %d: %s %d bits V:%d\n", i, ctrl_m->action_params[i]->name,ctrl_m->action_params[i]->length, *((uint8_t*)ctrl_m->action_params[i]->bitmap));
			/*ctrl_m->action_params[i]*/
		}   

	}
	

}

int main()
{
	int i=0;
	backend bg;
	digest d;
	uint8_t mac[6] = {0x00, 0x0d, 0x3f, 0xcd, 0x02, 0x5f}; /*00:0d:3f:cd:02:5f*/
	uint8_t port = 16;

	printf("Create backend...\n");
	bg = create_backend(3,1000,"localhost", 11111, cbf_test);

	printf("Launch backend...\n");
	launch_backend(bg);

	for(;i<2;++i) {
		printf("Wait...\n");
		sleep(1);
	}

	printf("Send a digest...\n");

	printf(" * Create a digest and add fields...\n");
	d = create_digest(bg, "mac_learn_digest");

/*	add_digest_field(d, "ethernet.srcAddr", mac, 48);
	add_digest_field(d, "standard_metadata.ingress_port", &port, 8);*/
	add_digest_field(d, mac, 48);
	add_digest_field(d, &port, 8);


	printf(" * Send the prepared digest to the receiver...\n");
	i = send_digest(bg, d, 0);

	printf(" * retval = %d\n", i);

	for (i=0;i<10;++i) {
		printf("Wait...\n");
		sleep(1);
	}

	printf("Stop backend...");
	stop_backend(bg);

	printf("Destroy backend...");
	destroy_backend(bg);

	return 0;
}

