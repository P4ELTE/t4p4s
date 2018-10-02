// Copyright 2018 Eotvos Lorand University, Budapest, Hungary
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


// This file is included directly from `dpdk_lib.c`.


extern int promiscuous_on;
extern int numa_on;

extern int check_lcore_params();


static void print_usage(const char *prgname)
{
    printf ("%s [EAL options] -- -p PORTMASK -P"
        "  [--config (port,queue,lcore)[,(port,queue,lcore]]"
        "  [--enable-jumbo [--max-pkt-len PKTLEN]]\n"
        "  -p PORTMASK: hexadecimal bitmask of ports to configure\n"
        "  -P : enable promiscuous mode\n"
        "  --config (port,queue,lcore): rx queues configuration\n"
        "  --no-numa: optional, disable numa awareness\n"
        "  --enable-jumbo: enable jumbo frame"
        " which max packet len is PKTLEN in decimal (64-9600)\n"
        "  --hash-entry-num: specify the hash entry number in hexadecimal to be setup\n",
        prgname);
}

static int parse_max_pkt_len(const char *pktlen)
{
    char *end = NULL;
    unsigned long len;

    /* parse decimal string */
    len = strtoul(pktlen, &end, 10);
    if ((pktlen[0] == '\0') || (end == NULL) || (*end != '\0'))
        return -1;

    if (len == 0)
        return -1;

    return len;
}

static int
parse_portmask(const char *portmask)
{
    char *end = NULL;
    unsigned long pm;

    /* parse hexadecimal string */
    pm = strtoul(portmask, &end, 16);
    if ((portmask[0] == '\0') || (end == NULL) || (*end != '\0'))
        return -1;

    if (pm == 0)
        return -1;

    return pm;
}

// Note: sets nb_lcore_params and lcore_params.
static int parse_lcore_params(const char *q_arg)
{
    char s[256];
    const char *p, *p0 = q_arg;
    char *end;
    enum fieldnames {
        FLD_PORT = 0,
        FLD_QUEUE,
        FLD_LCORE,
        _NUM_FLD
    };
    unsigned long int_fld[_NUM_FLD];
    char *str_fld[_NUM_FLD];

    nb_lcore_params = 0;

    while ((p = strchr(p0,'(')) != NULL) {
        ++p;
        if((p0 = strchr(p,')')) == NULL)
            return -1;

        unsigned size = p0 - p;
        if(size >= sizeof(s))
            return -1;

        snprintf(s, sizeof(s), "%.*s", size, p);
        if (rte_strsplit(s, sizeof(s), str_fld, _NUM_FLD, ',') != _NUM_FLD)
            return -1;
        for (int i = 0; i < _NUM_FLD; i++){
            errno = 0;
            int_fld[i] = strtoul(str_fld[i], &end, 0);
            if (errno != 0 || end == str_fld[i] || int_fld[i] > 255)
                return -1;
        }
        if (nb_lcore_params >= MAX_LCORE_PARAMS) {
            printf("exceeded max number of lcore params: %hu\n", nb_lcore_params);
            return -1;
        }
        lcore_params[nb_lcore_params].port_id = (uint8_t)int_fld[FLD_PORT];
        lcore_params[nb_lcore_params].queue_id = (uint8_t)int_fld[FLD_QUEUE];
        lcore_params[nb_lcore_params].lcore_id = (uint8_t)int_fld[FLD_LCORE];
        ++nb_lcore_params;
    }
    return 0;
}

#define CMD_LINE_OPT_CONFIG "config"
#define CMD_LINE_OPT_NO_NUMA "no-numa"
#define CMD_LINE_OPT_ENABLE_JUMBO "enable-jumbo"
#define CMD_LINE_OPT_HASH_ENTRY_NUM "hash-entry-num"

/* Parse the argument given in the command line of the application */
static int parse_args(int argc, char **argv)
{
    int opt, ret;
    char **argvopt;
    int option_index;
    char *prgname = argv[0];
    static struct option lgopts[] = {
        {CMD_LINE_OPT_CONFIG,         1, 0, 0},
        {CMD_LINE_OPT_NO_NUMA,        0, 0, 0},
        {CMD_LINE_OPT_ENABLE_JUMBO,   0, 0, 0},
        {CMD_LINE_OPT_HASH_ENTRY_NUM, 1, 0, 0},
        {NULL,                        0, 0, 0}
    };

    argvopt = argv;

    while ((opt = getopt_long(argc, argvopt, "p:P",
                lgopts, &option_index)) != EOF) {

        switch (opt) {
        /* portmask */
        case 'p':
            enabled_port_mask = parse_portmask(optarg);
            if (enabled_port_mask == 0) {
                printf("invalid portmask\n");
                print_usage(prgname);
                return -1;
            }
            break;
        case 'P':
            printf("Promiscuous mode on by default\n");
            break;

        /* long options */
        case 0:
            if (!strncmp(lgopts[option_index].name, CMD_LINE_OPT_CONFIG,
                sizeof (CMD_LINE_OPT_CONFIG))) {
                ret = parse_lcore_params(optarg);
                if (ret) {
                    printf("invalid config\n");
                    print_usage(prgname);
                    return -1;
                }
            }

            if (!strncmp(lgopts[option_index].name, CMD_LINE_OPT_NO_NUMA,
                sizeof(CMD_LINE_OPT_NO_NUMA))) {
                printf("numa is disabled \n");
                numa_on = 0;
            }
            break;

        default:
            print_usage(prgname);
            return -1;
        }
    }

    if (optind >= 0)
        argv[optind-1] = prgname;

    ret = optind-1;
    optind = 0; /* reset getopt lib */
    return ret;
}

void initialize_args(int argc, char **argv)
{
    /* init EAL */
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");
    argc -= ret;
    argv += ret;

    /* parse application arguments (after the EAL ones) */
    ret = parse_args(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Invalid T4P4S arguments\n");

    if (check_lcore_params() < 0)
        rte_exit(EXIT_FAILURE, "check_lcore_params failed\n");
}
