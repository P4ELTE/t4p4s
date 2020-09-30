// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#include "controller.h"
#include "messages.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>

controller c;

extern void notify_controller_initialized();

extern void set_table_default_action(const char* table_nickname, const char* table_name, const char* default_action_name);
extern void fill_t_fwd_table(uint16_t inport, uint16_t port, uint8_t mac[6], bool wmac);

void dhf(void* b) {
       printf("Unknown digest received\n");
}

bool uplink, downlink;
uint8_t uplink_mac[6];
uint8_t downlink_mac[6];
uint16_t uplink_port, downlink_port;
uint16_t uplink_inport, downlink_inport;

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

                if (9 == sscanf(line, "%d %d %d %x:%x:%x:%x:%x:%x", 
                                &inport, &port, &ismac, &values[0], &values[1], &values[2],
                                &values[3], &values[4], &values[5]) )
                {
                        if (port==0) //uplink
                        {
                                uplink = (ismac==1);
                                for( i = 0; i < 6; ++i )
                                        uplink_mac[i] = (uint8_t) values[i];
                                uplink_port = (uint16_t) port;
                                uplink_inport = (uint16_t) inport;
                                printf("UPLINK %d %d %x:%x:%x:%x:%x:%x\n", uplink_port, uplink_inport,values[0],values[1],values[2],values[3],values[4],values[5]);
                        }
                        else
                        {
                                downlink = (ismac==1);
                                for( i = 0; i < 6; ++i )
                                        downlink_mac[i] = (uint8_t) values[i];
                                downlink_port = (uint16_t) port;
                                downlink_inport = (uint16_t) inport;
                                printf("DOWNLINK %d %d %x:%x:%x:%x:%x:%x\n", downlink_port, downlink_inport,values[0],values[1],values[2],values[3],values[4],values[5]);
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
        set_table_default_action("t_fwd", "t_fwd_0", "_drop");

        fill_t_fwd_table(uplink_inport, uplink_port, uplink_mac, uplink);
        fill_t_fwd_table(downlink_inport, downlink_port, downlink_mac, downlink);

        notify_controller_initialized();
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

