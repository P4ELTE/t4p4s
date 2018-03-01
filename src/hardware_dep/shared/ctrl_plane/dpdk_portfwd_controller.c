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
#include <stdbool.h>

controller c;

void fill_t_fwd_table(uint16_t inport, uint8_t port, uint8_t mac[6], bool wmac)
{
        char buffer[2048];
        struct p4_header* h;
        struct p4_add_table_entry* te;
        struct p4_action* a;
        struct p4_action_parameter* ap;
        struct p4_action_parameter* ap2;
        struct p4_field_match_exact* exact;

/*	uint8_t v[2];
	v[0]=inport;
	v[1]=0;
*/
        h = create_p4_header(buffer, 0, 2048);
        te = create_p4_add_table_entry(buffer,0,2048);
	strcpy(te->table_name, "t_fwd");

        exact = add_p4_field_match_exact(te, 2048);
        strcpy(exact->header.name, "standard_metadata.ingress_port");
        memcpy(exact->bitmap, &inport , 2);
        exact->length = 2*8+0;
	if (wmac) {
        	a = add_p4_action(h, 2048);
        	strcpy(a->description.name, "forward_rewrite");
	
		ap = add_p4_action_parameter(h, a, 2048);	
		strcpy(ap->name, "port");
		memcpy(ap->bitmap, &port, 1);
		ap->length = 1*8+0;

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
		memcpy(ap->bitmap, &port, 1);
		ap->length = 1*8+0;

        	netconv_p4_header(h);
        	netconv_p4_add_table_entry(te);
        	netconv_p4_field_match_exact(exact);
        	netconv_p4_action(a);
		netconv_p4_action_parameter(ap);

	}

        send_p4_msg(c, buffer, 2048);
}

void dhf(void* b) {
       printf("Unknown digest received\n");
}

void set_default_action_t_fwd()
{
	char buffer[2048];
	struct p4_header* h;
	struct p4_set_default_action* sda;
	struct p4_action* a;
	struct p4_action_parameter* ap;
        uint8_t port = 1;

	printf("Generate set_default_action message for table t_fwd\n");
	
	h = create_p4_header(buffer, 0, sizeof(buffer));

	sda = create_p4_set_default_action(buffer,0,sizeof(buffer));
	strcpy(sda->table_name, "t_fwd");

	a = &(sda->action);
	strcpy(a->description.name, "_drop");

//        strcpy(a->description.name, "forward");
	
//	ap = add_p4_action_parameter(h, a, 2048);	
//	strcpy(ap->name, "port");
//	memcpy(ap->bitmap, &port, 1);
//	ap->length = 1*8+0;

	netconv_p4_header(h);
	netconv_p4_set_default_action(sda);
	netconv_p4_action(a);
//	netconv_p4_action_parameter(ap);

	send_p4_msg(c, buffer, sizeof(buffer));
}

bool uplink, downlink;
uint8_t uplink_mac[6];
uint8_t downlink_mac[6];
uint8_t uplink_port, downlink_port;
uint8_t uplink_inport, downlink_inport;

int read_config_from_file(char *filename) {
        FILE *f;
        char line[100];
	int values[6];
	int port, inport;
	int ismac;
	int i;

        f = fopen(filename, "r");
        if (f == NULL) return -1;

        while (fgets(line, sizeof(line), f)) {
                line[strlen(line)-1] = '\0';

		if (9 == sscanf(line, "%d %d %d %x:%x:%x:%x:%x:%x%c", 
				&inport, &port, &ismac, &values[0], &values[1], &values[2],
				&values[3], &values[4], &values[5]) )
		{
			if (port==0) //uplink
			{
				uplink = (ismac==1);
				for( i = 0; i < 6; ++i )
					uplink_mac[i] = (uint8_t) values[i];
				uplink_port = (uint8_t) port;
				uplink_inport = (uint8_t) inport;
		
			}
			else
			{
				downlink = (ismac==1);
				for( i = 0; i < 6; ++i )
					downlink_mac[i] = (uint8_t) values[i];
				downlink_port = (uint8_t) port;
				downlink_inport = (uint8_t) inport;
			}

		} else {
			printf("Wrong format error in line\n");
			fclose(f);
			return -1;
		}
		
        }

        fclose(f);
        return 0;
}

void init() {
	printf("Set default actions.\n");
	set_default_action_t_fwd();

	fill_t_fwd_table(uplink_inport, uplink_port, uplink_mac, uplink);
	fill_t_fwd_table(downlink_inport, downlink_port, downlink_mac, downlink);
}


int main(int argc, char* argv[]) 
{
        if (argc>1) {
                if (argc!=2) {
                        printf("Too many arguments...\nUsage: %s <filename(optional)>\n", argv[0]);
                        return -1;
                }
                printf("Command line argument is present...\nLoading configuration data...\n");
                if (read_config_from_file(argv[1])<0) {
                        printf("File cannnot be opened...\n");
                        return -1;
                }
        }

	printf("Create and configure controller...\n");
	c = create_controller_with_init(11111, 3, dhf, init);

	printf("Launching controller's main loop...\n");
	execute_controller(c);

	printf("Destroy controller\n");
	destroy_controller(c);

	return 0;
}

