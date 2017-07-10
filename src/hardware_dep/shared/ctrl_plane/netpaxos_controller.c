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

controller c;

///////////////////////////////////////////////////////////////////////////////////////////////////
///                                 NETPAXOS FILL TABLES:
///////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum {FORWARD_TBL_DROP, FORWARD_TBL_FORWARD} forward_tbl_action;

// FORWARD
void fill_forward_tbl(forward_tbl_action taction, uint8_t ingress_port, uint8_t fwd_port)
{
    char buffer[2048];
    struct p4_header* h;
    struct p4_add_table_entry* te;
    struct p4_action* a;
    struct p4_action_parameter* ap;
    struct p4_field_match_exact* exact;

    printf("Fill table forward_tbl...\n");

    h = create_p4_header(buffer, 0, 2048);

    // 1. Set the table name in which you want to insert a new entry
    te = create_p4_add_table_entry(buffer,0,2048);
    strcpy(te->table_name, "forward_tbl");

    // 2. According to the match type, fill the key values. If there are multiple "reads" in the table, further keys should be added similarly
    exact = add_p4_field_match_exact(te, 2048);
    strcpy(exact->header.name, "standard_metadata.ingress_port");
    memcpy(exact->bitmap, &ingress_port, 1);
    exact->length = 1*8+0;

    // 3. Define the action for the match rules added previously
    a = add_p4_action(h, 2048);

    switch(taction) {
	    case FORWARD_TBL_DROP:
		// 4. Fill the action name and parameters. Drop has no params.
		strcpy(a->description.name, "_drop");
		break;
	    case FORWARD_TBL_FORWARD:
		// 4. Fill the action name and parameters.
		strcpy(a->description.name, "forward");
		// 4b. Add action parameters. 
		ap = add_p4_action_parameter(h, a, 2048);   
		strcpy(ap->name, "port");
		memcpy(ap->bitmap, &fwd_port, 1);
		ap->length = 1*8+0;
    }

    // 5. Byte order conversions
    netconv_p4_header(h);
    netconv_p4_add_table_entry(te);
    netconv_p4_field_match_exact(exact);
    netconv_p4_action(a);
    if (taction == FORWARD_TBL_FORWARD) netconv_p4_action_parameter(ap);

    // 6. Sending the message to the switch
    send_p4_msg(c, buffer, 2048);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void dhf(void* b) {
    struct p4_header* h = netconv_p4_header(unpack_p4_header(b, 0));
    if (h->type != P4T_DIGEST) {
        printf("Method is not implemented\n");
        return;
    }

    struct p4_digest* d = unpack_p4_digest(b,0);
    printf("Unknown digest received: X%sX\n", d->field_list_name);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///                                 NETPAXOS DEFAULT ACTIONS:
///////////////////////////////////////////////////////////////////////////////////////////////////

// FORWARD_TBL
void set_default_action_forward_tbl() //forward
{
    char buffer[2048];
    struct p4_header* h;
    struct p4_set_default_action* sda;
    struct p4_action* a;
    struct p4_action_parameter* ap;
    uint8_t default_port = 0; // <----------------------- YOU SHOULD UPDATE IT IF NEEDED

    printf("Generate set_default_action message for table NetPaxos-->forward_tbl\n");
    
    h = create_p4_header(buffer, 0, sizeof(buffer));

    // 1. Set the table name
    sda = create_p4_set_default_action(buffer,0,sizeof(buffer));
    strcpy(sda->table_name, "forward_tbl");

    // 2. Set the name of default action
    a = &(sda->action);
    strcpy(a->description.name, "forward");

    // 3. Add the parameters (for more than one parameters this section should be repeated...)
    ap = add_p4_action_parameter(h, a, 2048);
    strcpy(ap->name, "port");
    memcpy(ap->bitmap, &default_port, 1);
    ap->length = 1*8+0;

    // 4. Byte order conversions
    netconv_p4_header(h);
    netconv_p4_set_default_action(sda);
    netconv_p4_action(a);
    netconv_p4_action_parameter(ap);

    // 5. Sending the message
    send_p4_msg(c, buffer, sizeof(buffer));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void init() {
    printf("Set default actions.\n");

    set_default_action_forward_tbl(); //forward

    fill_forward_tbl(FORWARD_TBL_FORWARD, 0, 1); //forward ingress port=0; port parameter of action forward is 1
    
}


int main(int argc, char* argv[]) 
{
        if (argc>1) {
                        printf("Too many arguments...\nUsage: %s <filename(optional)>\n", argv[0]);
                        return -1;
        }

    printf("Create and configure NetPaxos controller...\n");
    c = create_controller_with_init(11111, 3, dhf, init);

    printf("Launching controller's main loop...\n");
    execute_controller(c);

    printf("Destroy NetPaxos controller\n");
    destroy_controller(c);

    return 0;
}

