// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#include "controller.h"
#include "messages.h"
#include "dpdk_controller_dictionary.h"
#include "dpdk_ctrl_common.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#define MAX_MACS 60000

controller c;

void mac_learn_digest(void* digest) {
    digest_macport_t dig;
    undigest_macport(&dig, digest);

    send_exact_entry(dig.port, dig.mac, "dmac", "ethernet.dstAddr", "forward", "port", 0);
    send_exact_entry(dig.port, dig.mac, "smac", "ethernet.srcAddr", "_nop", 0, 0);
}

void test_learn_ip(void* digest) {
    digest_ip_t dig;
    undigest_ip(&dig, digest);
    send_lpm_entry(dig.ip, dig.prefix_length, "table1", "field1", "korte", dig.i1, dig.i2, dig.i3);
}

digest_dispatch_t dispatchers[] = {
    { "mac_learn_digest", &mac_learn_digest },
    { "test_learn_ip",    &test_learn_ip },
    { "", 0 }
};

void dhf(void* b) {
    struct p4_header* h = netconv_p4_header(unpack_p4_header(b, 0));
    if (h->type != P4T_DIGEST) {
        printf("Method is not implemented\n");
        return;
    }

    struct p4_digest* d = unpack_p4_digest(b,0);

    for (int digest_idx = 0; dispatchers[digest_idx].digest_fun != 0; ++digest_idx) {
        if (strcmp(d->field_list_name, dispatchers[digest_idx].name) == 0) {
            dispatchers[digest_idx].digest_fun(b);
            return;
        }
    }

    printf("Unknown digest received: %s\n", d->field_list_name);
}


int process_translation(const char* line) {
    char key[100];
    char value[100];
    int matches = sscanf(line, "%*s %s %s", key, value);
    if (2 != matches) return -1;

    add_translation(key, value);
    printf("Translation %s -> %s\n", key, value);

    return 0;
}

int process_set_default(const char* line) {
    char table_name[100];
    char default_action_name[100];

    int matches = sscanf(line, "%*s %s %s", table_name, default_action_name);
    if (2 != matches) return -1;

    set_table_default_action(table_name, table_name, default_action_name);
    return 0;
}

int process_mac_fwd(const char* line) {
    uint16_t port;
    uint8_t mac[6];

    int matches = sscanf(line, "%*s %hhx:%hhx:%hhx:%hhx:%hhx:%hhx %hd", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5], &port);
    if (7 != matches) return -1;

    send_exact_entry(port, mac, "dmac", "ethernet.dstAddr", "forward", "port", 0);
    return send_exact_entry(port, mac, "smac", "ethernet.srcAddr", "_nop", 0, 0);
}

int process_exact(const char* line) {
    uint16_t port;
    uint8_t mac[6];

    char table_name[100];
    char header_name[100];
    char action_name[100];
    char par1[100];
    char par2[100];

    int matches = sscanf(line, "%*s %hhx:%hhx:%hhx:%hhx:%hhx:%hhx %hd %s %s %s %s %s", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5], &port, table_name, header_name, action_name, par1, par2);
    if (6 + 3 > matches) return -1;

    return send_exact_entry(port, mac, table_name, header_name, action_name, matches < 6+3+1 ? 0 : par1, matches < 6+3+1 ? 0 : par2);
}

int process_ue_selector(const char* line) {
    return -1;
}

int process_teid_rate_limiter(const char* line) {
    uint32_t teid;
    char table_name[100];
    char header_name[100];
    char teid_mode[100];

    int matches = sscanf(line, "%*s %d %s %s %s", &teid, table_name, header_name, teid_mode);
    if (4 != matches) return -1;

    return fill_teid_rate_limiter_table(teid, table_name, header_name, teid_mode);
}

int process_m_filter(const char* line) {
    return -1;
}

int process_smac_dmac(const char* line) {
    return -1;
}

int process_nexthop(const char* line) {
    return -1;
}

int process_uplink(const char* line) {
    char table_name[100];
    char header_name[100];
    char action_name[100];

    uint8_t mac[6];
    uint16_t port;
    uint16_t inport;

    int matches = sscanf(line, "%*s %s %s %s %hhx:%hhx:%hhx:%hhx:%hhx:%hhx %hd %hd", 
                                table_name, header_name, action_name,
                                &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5], &inport, &port);
    if (3+6+2 != matches) return -1;

    // return send_exact_entry(inport, mac, table_name, header_name, action_name, port, 0);
    return -1;
}

int process_downlink(const char* line) {
    char table_name[100];
    char header_name[100];
    char action_name[100];

    uint8_t mac[6];
    uint16_t port;
    uint16_t inport;

    int matches = sscanf(line, "%*s %s %s %s %hhx:%hhx:%hhx:%hhx:%hhx:%hhx %hd %hd", 
                                table_name, header_name, action_name,
                                &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5], &inport, &port);
    if (3+6+2 != matches) return -1;

    // return send_exact_entry(inport, mac, table_name, header_name, action_name, port, 0);
    return -1;
}

typedef int (*config_line_processor_t)(const char* line);

typedef struct config_processor_s {
    char name[64];
    config_line_processor_t process;
} config_processor_t;

config_processor_t code_processors[] = {
    { "TRANSLATION",       process_translation },
    { "SET-DEFAULT",       process_set_default },
    { "MAC-FWD",           process_mac_fwd },
    { "EXACT",             process_exact },

    { "UE-SELECTOR",       process_ue_selector },
    { "TEID-RATE-LIMITER", process_teid_rate_limiter },
    { "M-FILTER",          process_m_filter },
    { "SMAC-DMAC",         process_smac_dmac },
    { "NEXTHOP",           process_nexthop },

    { "UPLINK",            process_uplink },
    { "DOWNLINK",          process_downlink },

    { {0}, 0 },
};


int process_config_file(FILE *f) {
    char line[100];
    char format_code[256];

    int line_index = 0;
    while (fgets(line, sizeof(line), f)) {
        line[strlen(line)-1] = '\0';
        ++line_index;

        // skipping empty lines and comments (lines that do not begin with a format code)
        // if (0 == strlen(line))   continue;
        // if (0 == sscanf(line, "%*[^a-zA-Z]"))   continue;

        int error_code = (1 == sscanf(line, "%s ", format_code));
        if (!error_code) {
            printf("Line %d: missing format code, skipping\n", line_index);
            continue;
        }

        for (int i = 0; code_processors[i].process != 0; ++i) {
            if (strcmp(code_processors[i].name, format_code) == 0) {
                error_code = code_processors[i].process(line);
                // "continue"s outer loop
                goto outer_loop_end;
            }
        }

        printf("Line %d: unknown format code %s\n", line_index, format_code);
        continue;

        outer_loop_end:
            if (error_code != 0) {
                printf("Line %d: could not process line (error code %d): %s\n", line_index, error_code, line);
            }
    }

    fclose(f);
    return 0;
}


#define MAX_CONFIG_FILES 64
static FILE* config_files[MAX_CONFIG_FILES] = { 0 };
static char config_file_names[100][MAX_CONFIG_FILES] = { "" };

void init() {
    for (int i = 1; i < MAX_CONFIG_FILES; ++i) {
        if (config_files[i] == 0)   break;
        printf("Processing config file %s\n", config_file_names[i]);
        process_config_file(config_files[i]);
    }

    printf("Done processing all config files\n");

    notify_controller_initialized();
}


int init_args(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        printf("Opening config file %s\n", argv[i]);
        config_files[i] = fopen(argv[i], "r");
        if (config_files[i] == 0) {
            printf("Error: cannot open config file %s\n", argv[i]);
            return -1;
        }
        strcpy(config_file_names[i], argv[i]);
        printf("Copied %s\n", config_file_names[i]);
    }


    printf("All config files opened\n");

    return 0;
}

int main(int argc, char* argv[])
{
    printf("Controller main started\n");

    int error_code = init_args(argc, argv);
    if (error_code < 0)    return error_code;

    printf("Create and configure controller...\n");
    c = create_controller_with_init(11111, 3, dhf, init);

    printf("Launching controller's main loop...\n");
    execute_controller(c);

    printf("Destroy controller\n");
    destroy_controller(c);

    return 0;
}
