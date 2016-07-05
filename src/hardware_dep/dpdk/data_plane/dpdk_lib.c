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
#include "dpdk_lib.h"
#include "backend.h"
#include "ctrl_plane_backend.h"
#include "dataplane.h"
#include <unistd.h>

#include <rte_ether.h>
#include <rte_ethdev.h>

struct socket_state state[NB_SOCKETS];

//=   shared   ================================================================

extern struct rte_mempool * pktmbuf_pool[NB_SOCKETS];
extern struct rte_mempool *header_pool, *clone_pool;
extern struct lcore_conf lcore_conf[RTE_MAX_LCORE];


extern uint32_t enabled_port_mask;
int promiscuous_on = 0; /**< Ports set in promiscuous mode off by default. */
int numa_on = 1; /**< NUMA is enabled by default. */

#define MAX_LCORE_PARAMS 1024
struct lcore_params {
	uint8_t port_id;
	uint8_t queue_id;
	uint8_t lcore_id;
} __rte_cache_aligned;
struct lcore_params lcore_params_array[MAX_LCORE_PARAMS];
struct lcore_params lcore_params_array_default[];
struct lcore_params * lcore_params;
uint16_t nb_lcore_params;
struct lcore_params lcore_params_array[MAX_LCORE_PARAMS];
struct lcore_params lcore_params_array_default[] = {
	{0, 0, 2},
	{0, 1, 2},
	{0, 2, 2},
	{1, 0, 2},
	{1, 1, 2},
	{1, 2, 2},
	{2, 0, 2},
	{3, 0, 3},
	{3, 1, 3},
};

struct lcore_params * lcore_params = lcore_params_array_default;
uint16_t nb_lcore_params = sizeof(lcore_params_array_default) /
				sizeof(lcore_params_array_default[0]);

struct l2fwd_port_statistics port_statistics[RTE_MAX_ETHPORTS];

//=   used only here   ========================================================

struct ether_addr ports_eth_addr[RTE_MAX_ETHPORTS];

unsigned int rx_queue_per_lcore = 1;

#define MAX_JUMBO_PKT_LEN  9600

#define RTE_TEST_RX_DESC_DEFAULT 128
#define RTE_TEST_TX_DESC_DEFAULT 512
#ifndef FAKEDPDK
static uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
static uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;
#endif

static struct rte_eth_conf port_conf = {
	.rxmode = {
		.mq_mode = ETH_MQ_RX_RSS,
		.max_rx_pkt_len = ETHER_MAX_LEN,
		.split_hdr_size = 0,
		.header_split   = 0, /**< Header Split disabled */
		.hw_ip_checksum = 1, /**< IP checksum offload enabled */
		.hw_vlan_filter = 0, /**< VLAN filtering disabled */
		.jumbo_frame    = 0, /**< Jumbo Frame Support disabled */
		.hw_strip_crc   = 1, /**< CRC stripped by hardware */
        .mq_mode = ETH_MQ_RX_RSS, 
	},
	.rx_adv_conf = {
		.rss_conf = {
			.rss_key = NULL,
			.rss_hf = ETH_RSS_IP,
		},
	},
	.txmode = {
		.mq_mode = ETH_MQ_TX_NONE,
	},
};

//=============================================================================

#define UNUSED(x) (void)(x)

static int
check_lcore_params(void)
{
	uint8_t lcore;
	uint16_t i;
	int socketid;

	for (i = 0; i < nb_lcore_params; ++i) {
		uint8_t queue = lcore_params[i].queue_id;

		if (queue >= MAX_RX_QUEUE_PER_PORT) {
			printf("queue number %hhu over maximum receiving queues per port (%hhu)\n", queue, MAX_RX_QUEUE_PER_PORT);
			return -1;
		}

		lcore = lcore_params[i].lcore_id;

		if (!rte_lcore_is_enabled(lcore)) {
			printf("error: lcore %hhu is not enabled in lcore mask\n", lcore);
			return -1;
		}

		if ((socketid = rte_lcore_to_socket_id(lcore) != 0) &&
			(numa_on == 0)) {
			printf("warning: lcore %hhu is on socket %d with numa off \n",
				lcore, socketid);
		}
	}
	return 0;
}

static int
check_port_config(const unsigned nb_ports)
{
	unsigned portid;
	uint16_t i;

	for (i = 0; i < nb_lcore_params; ++i) {
		portid = lcore_params[i].port_id;

		if ((enabled_port_mask & (1 << portid)) == 0) {
			printf("port %u is not enabled in port mask\n", portid);
			return -1;
		}
		if (portid >= nb_ports) {
			printf("port %u is not present on the board\n", portid);
			return -1;
		}
	}
	return 0;
}

static uint8_t
get_port_n_rx_queues(const uint8_t port)
{
	int queue = -1;
	uint16_t i;

	for (i = 0; i < nb_lcore_params; ++i) {
		if (lcore_params[i].port_id == port && lcore_params[i].queue_id > queue)
			queue = lcore_params[i].queue_id;
	}
	return (uint8_t)(++queue);
}

static int
init_lcore_rx_queues(void)
{
	uint16_t i, nb_rx_queue;
	uint8_t lcore;

	for (i = 0; i < nb_lcore_params; ++i) {
		lcore = lcore_params[i].lcore_id;
		nb_rx_queue = lcore_conf[lcore].n_rx_queue;

		if (nb_rx_queue >= MAX_RX_QUEUE_PER_LCORE) {
			printf("error: too many queues (%u) for lcore %u (max: %u)\n",
				(unsigned)nb_rx_queue + 1, (unsigned)lcore, MAX_RX_QUEUE_PER_LCORE);
			return -1;
		}

		lcore_conf[lcore].rx_queue_list[nb_rx_queue].port_id =
			lcore_params[i].port_id;
		lcore_conf[lcore].rx_queue_list[nb_rx_queue].queue_id =
			lcore_params[i].queue_id;
		lcore_conf[lcore].n_rx_queue++;
	}
	return 0;
}

/* display usage */
static void
print_usage(const char *prgname)
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

static int
parse_config(const char *q_arg)
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
	int i;
	unsigned size;

	nb_lcore_params = 0;

	while ((p = strchr(p0,'(')) != NULL) {
		++p;
		if((p0 = strchr(p,')')) == NULL)
			return -1;

		size = p0 - p;
		if(size >= sizeof(s))
			return -1;

		snprintf(s, sizeof(s), "%.*s", size, p);
		if (rte_strsplit(s, sizeof(s), str_fld, _NUM_FLD, ',') != _NUM_FLD)
			return -1;
		for (i = 0; i < _NUM_FLD; i++){
			errno = 0;
			int_fld[i] = strtoul(str_fld[i], &end, 0);
			if (errno != 0 || end == str_fld[i] || int_fld[i] > 255)
				return -1;
		}
		if (nb_lcore_params >= MAX_LCORE_PARAMS) {
			printf("exceeded max number of lcore params: %hu\n",
				nb_lcore_params);
			return -1;
		}
		lcore_params_array[nb_lcore_params].port_id = (uint8_t)int_fld[FLD_PORT];
		lcore_params_array[nb_lcore_params].queue_id = (uint8_t)int_fld[FLD_QUEUE];
		lcore_params_array[nb_lcore_params].lcore_id = (uint8_t)int_fld[FLD_LCORE];
		++nb_lcore_params;
	}
	lcore_params = lcore_params_array;
	return 0;
}

#define CMD_LINE_OPT_CONFIG "config"
#define CMD_LINE_OPT_NO_NUMA "no-numa"
#define CMD_LINE_OPT_ENABLE_JUMBO "enable-jumbo"
#define CMD_LINE_OPT_HASH_ENTRY_NUM "hash-entry-num"

/* Parse the argument given in the command line of the application */
static int
parse_args(int argc, char **argv)
{
	int opt, ret;
	char **argvopt;
	int option_index;
	char *prgname = argv[0];
	static struct option lgopts[] = {
		{CMD_LINE_OPT_CONFIG, 1, 0, 0},
		{CMD_LINE_OPT_NO_NUMA, 0, 0, 0},
		{CMD_LINE_OPT_ENABLE_JUMBO, 0, 0, 0},
		{CMD_LINE_OPT_HASH_ENTRY_NUM, 1, 0, 0},
		{NULL, 0, 0, 0}
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
			printf("Promiscuous mode selected\n");
			promiscuous_on = 1;
			break;

		/* long options */
		case 0:
			if (!strncmp(lgopts[option_index].name, CMD_LINE_OPT_CONFIG,
				sizeof (CMD_LINE_OPT_CONFIG))) {
				ret = parse_config(optarg);
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

			if (!strncmp(lgopts[option_index].name, CMD_LINE_OPT_ENABLE_JUMBO,
				sizeof (CMD_LINE_OPT_ENABLE_JUMBO))) {
				struct option lenopts = {"max-pkt-len", required_argument, 0, 0};

				printf("jumbo frame is enabled - disabling simple TX path\n");
				port_conf.rxmode.jumbo_frame = 1;

				/* if no max-pkt-len set, use the default value ETHER_MAX_LEN */
				if (0 == getopt_long(argc, argvopt, "", &lenopts, &option_index)) {
					ret = parse_max_pkt_len(optarg);
					if ((ret < 64) || (ret > MAX_JUMBO_PKT_LEN)){
						printf("invalid packet length\n");
						print_usage(prgname);
						return -1;
					}
					port_conf.rxmode.max_rx_pkt_len = ret;
				}
				printf("set jumbo frame max packet length to %u\n",
						(unsigned int)port_conf.rxmode.max_rx_pkt_len);
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

static void
print_ethaddr(const char *name, const struct ether_addr *eth_addr)
{
	char buf[ETHER_ADDR_FMT_SIZE];
	ether_format_addr(buf, ETHER_ADDR_FMT_SIZE, eth_addr);
	printf("%s%s", name, buf);
}

/* Check the link status of all ports in up to 9s, and print them finally */
static void
check_all_ports_link_status(uint8_t port_num, uint32_t port_mask)
{
#define CHECK_INTERVAL 100 /* 100ms */
#define MAX_CHECK_TIME 90 /* 9s (90 * 100ms) in total */
	uint8_t portid, count, all_ports_up, print_flag = 0;
	struct rte_eth_link link;

	printf("\nChecking link status");
	fflush(stdout);
	for (count = 0; count <= MAX_CHECK_TIME; count++) {
		all_ports_up = 1;
		for (portid = 0; portid < port_num; portid++) {
			if ((port_mask & (1 << portid)) == 0)
				continue;
			memset(&link, 0, sizeof(link));
			rte_eth_link_get_nowait(portid, &link);
			/* print link status if flag set */
			if (print_flag == 1) {
				if (link.link_status)
					printf("Port %d Link Up - speed %u "
						"Mbps - %s\n", (uint8_t)portid,
						(unsigned)link.link_speed,
				(link.link_duplex == ETH_LINK_FULL_DUPLEX) ?
					("full-duplex") : ("half-duplex\n"));
				else
					printf("Port %d Link Down\n",
						(uint8_t)portid);
				continue;
			}
			/* clear all_ports_up flag if any link down */
			if (link.link_status == 0) {
				all_ports_up = 0;
				break;
			}
		}
		/* after finally printing all link status, get out */
		if (print_flag == 1)
			break;

		if (all_ports_up == 0) {
			printf(".");
			fflush(stdout);
			rte_delay_ms(CHECK_INTERVAL);
		}

		/* set the print_flag if all ports up or timeout */
		if (all_ports_up == 1 || count == (MAX_CHECK_TIME - 1)) {
			print_flag = 1;
			printf("done\n");
		}
	}
}

static void
create_table(lookup_table_t* t, int socketid)
{
    t->socketid = socketid;
    if(t->key_size == 0) return; // we don't create the table if there are no keys (it's a fake table for an element in the pipeline)
    switch(t->type)
    {
        case LOOKUP_EXACT:
            exact_create(t, socketid);
            break;
        case LOOKUP_LPM:
            lpm_create(t, socketid);
            break;
        case LOOKUP_TERNARY:
            ternary_create(t, socketid);
            break;
    }
    debug("Created table %s on socket %d.\n", t->name, socketid);
}

static void
create_tables_on_socket(int socketid)
{
    if(table_config == NULL) return;
    debug("Initializing tables on socket %d...\n", socketid);
    int i;
    for(i = 0; i < NB_TABLES; i++) {
        lookup_table_t t = table_config[i];
        debug("Creating instances for table %s on socket %d (%d copies)\n", t.name, socketid, NB_REPLICA);
        int j;
        for(j = 0; j < NB_REPLICA; j++) {
            state[socketid].tables[i][j] = malloc(sizeof(lookup_table_t));
            memcpy(state[socketid].tables[i][j], &t, sizeof(lookup_table_t));
            state[socketid].tables[i][j]->instance = j;
            create_table(state[socketid].tables[i][j], socketid);
        }
        state[socketid].active_replica[i] = 0;
    }
}

static int
get_socketid(unsigned lcore_id)
{
	if (numa_on)
		return rte_lcore_to_socket_id(lcore_id);
	else
		return 0;
}

static void
create_counters_on_socket(int socketid)
{
    if(counter_config == NULL) return;
    debug("Initializing counters on socket %d...\n", socketid);
    int i;
    unsigned lcore_id;
    for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
        if (rte_lcore_is_enabled(lcore_id) == 0) continue; 
        if(socketid != get_socketid(lcore_id)) continue;
        for(i = 0; i < NB_COUNTERS; i++) {
            debug("Creating counter %d on socket %d lcore %d\n", i, socketid, lcore_id);
            counter_t c = counter_config[i];
            state[socketid].counters[i][lcore_id] = malloc(sizeof(counter_t));
            memcpy(state[socketid].counters[i][lcore_id], &c, sizeof(counter_t));
            debug("Initializing counter %d on socket %d lcore %d\n", i, socketid, lcore_id);
            state[socketid].counters[i][lcore_id]->values = (vector_t*)rte_malloc_socket("vector_t", sizeof(vector_t), 0, socketid);
            vector_init(state[socketid].counters[i][lcore_id]->values, 1 /* initial size */, c.size, sizeof(rte_atomic32_t), (void (*)(void *))&rte_atomic32_init, socketid);
        }
    }
}

static int
init_mem(unsigned nb_mbuf)
{
	unsigned lcore_id;
	char s[64];

	for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
		if (rte_lcore_is_enabled(lcore_id) == 0)
			continue;

		int socketid = get_socketid(lcore_id);

		if (socketid >= NB_SOCKETS) {
			rte_exit(EXIT_FAILURE, "Socket %d of lcore %u is out of range %d\n",
				socketid, lcore_id, NB_SOCKETS);
		}
		if (pktmbuf_pool[socketid] == NULL) {
			snprintf(s, sizeof(s), "mbuf_pool_%d", socketid);
			pktmbuf_pool[socketid] =
				rte_mempool_create(s, nb_mbuf, MBUF_SIZE, MEMPOOL_CACHE_SIZE,
					sizeof(struct rte_pktmbuf_pool_private),
					rte_pktmbuf_pool_init, NULL,
					rte_pktmbuf_init, NULL,
					socketid, 0);
			if (pktmbuf_pool[socketid] == NULL)
				rte_exit(EXIT_FAILURE,
						"Cannot init mbuf pool on socket %d\n", socketid);
			else
				printf("Allocated mbuf pool on socket %d\n", socketid);
		}
	}
	return 0;
}

int init_memories()
{
    debug("Initializing stateful memories...\n");
	int socketid;
	unsigned lcore_id;
	for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
		if (rte_lcore_is_enabled(lcore_id) == 0) continue;
		if (numa_on) socketid = rte_lcore_to_socket_id(lcore_id);
		else socketid = 0;
		if (socketid >= NB_SOCKETS) {
			rte_exit(EXIT_FAILURE, "Socket %d of lcore %u is out of range %d\n",
				socketid, lcore_id, NB_SOCKETS);
		}
		if (state[socketid].tables[0][0] == NULL) {
            create_tables_on_socket(socketid);
            create_counters_on_socket(socketid);
        }
	}
	return 0;
}

int init_lcore_confs()
{
    debug("Configuring lcore structs...\n");
	struct lcore_conf *qconf;
	int socketid;
	unsigned lcore_id;
	for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
		if (rte_lcore_is_enabled(lcore_id) == 0) continue;
		if (numa_on) socketid = rte_lcore_to_socket_id(lcore_id);
		else socketid = 0;
		if (socketid >= NB_SOCKETS) {
			rte_exit(EXIT_FAILURE, "Socket %d of lcore %u is out of range %d\n",
				socketid, lcore_id, NB_SOCKETS);
		}
		qconf = &lcore_conf[lcore_id];
        int i;
        for(i = 0; i < NB_TABLES; i++)
    		qconf->state.tables[i] = state[socketid].tables[i][0];
        for(i = 0; i < NB_COUNTERS; i++)
    		qconf->state.counters[i] = state[socketid].counters[i][lcore_id];
	}
	return 0;
}

static void change_replica(int socketid, int tid, int replica) {
    struct lcore_conf *qconf;
    int core_socketid;
    unsigned lcore_id;
    for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
        if (rte_lcore_is_enabled(lcore_id) == 0) continue;
        core_socketid = rte_lcore_to_socket_id(lcore_id);
        if(core_socketid != socketid) continue;
        qconf = &lcore_conf[lcore_id];
   	    qconf->state.tables[tid] = state[socketid].tables[tid][replica]; // TODO should this be atomic?
        state[socketid].active_replica[tid] = replica;
        //printf("\n\n\nCHANGING REPLICA of TABLE %d: core %d on socket %d now uses replica %d\n\n\n", tid, lcore_id, socketid, replica);
    }
}

#define CHANGE_TABLE(fun, par...) \
{ \
    int current_replica = state[socketid].active_replica[tableid]; \
    int next_replica = (current_replica+1)%NB_REPLICA; \
    fun(state[socketid].tables[tableid][next_replica], par); \
    change_replica(socketid, tableid, next_replica); \
    usleep(TABCHANGE_DELAY); \
    for(current_replica = 0; current_replica < NB_REPLICA; current_replica++) \
        if(current_replica != next_replica) fun(state[socketid].tables[tableid][current_replica], par); \
}

#define FORALLNUMANODES(b) \
    int socketid; \
    for(socketid = 0; socketid < NB_SOCKETS; socketid++) \
        if(state[socketid].tables[0][0] != NULL) \
            b
void
exact_add_promote(int tableid, uint8_t* key, uint8_t* value) {
    FORALLNUMANODES(CHANGE_TABLE(exact_add, key, value))
}
void
lpm_add_promote(int tableid, uint8_t* key, uint8_t depth, uint8_t* value) {
    FORALLNUMANODES(CHANGE_TABLE(lpm_add, key, depth, value))
}
void
ternary_add_promote(int tableid, uint8_t* key, uint8_t* mask, uint8_t* value) {
    FORALLNUMANODES(CHANGE_TABLE(ternary_add, key, mask, value))
}
void
table_setdefault_promote(int tableid, uint8_t* value) {
    FORALLNUMANODES(CHANGE_TABLE(table_setdefault, value))
}

void
increase_counter(int counterid, int index) {
    rte_atomic32_t* aint32;
    struct lcore_conf *conf;
    int lcore_id = rte_lcore_id();
    conf = &lcore_conf[lcore_id];
    aint32 = (rte_atomic32_t*)vector_get(conf->state.counters[counterid]->values, index);
    rte_atomic32_inc(aint32);
}

uint32_t
read_counter(int counterid, int index) {
    rte_atomic32_t* aint32;
    struct lcore_conf *conf;
    unsigned lcore_id;
    uint32_t cnt = 0;
    for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
        if (rte_lcore_is_enabled(lcore_id) == 0) continue;
        conf = &lcore_conf[lcore_id];
        aint32 = (rte_atomic32_t*)vector_get(conf->state.counters[counterid]->values, index);
        cnt += rte_atomic32_read(aint32);
    }
    return cnt;
}

uint8_t
initialize(int argc, char **argv)
{
	int ret;
	uint8_t nb_ports = 0;

	/* init EAL */
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");
	argc -= ret;
	argv += ret;

	/* parse application arguments (after the EAL ones) */
	ret = parse_args(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid P4_FWD arguments\n");

        if (check_lcore_params() < 0)
                rte_exit(EXIT_FAILURE, "check_lcore_params failed\n");

#ifndef FAKEDPDK
	struct lcore_conf *qconf;
/*	uint8_t nb_ports_available;*/
	uint8_t portid, queue;
	unsigned lcore_id;
	uint8_t socketid, nb_rx_queue;
	uint32_t n_tx_queue, nb_lcores;
        struct rte_eth_dev_info dev_info;
        struct rte_eth_txconf *txconf;
 	uint16_t queueid;

        ret = init_lcore_rx_queues();
        if (ret < 0)
                rte_exit(EXIT_FAILURE, "init_lcore_rx_queues failed\n");

        nb_ports = rte_eth_dev_count();
        if (nb_ports > RTE_MAX_ETHPORTS)
                nb_ports = RTE_MAX_ETHPORTS;

        if (check_port_config(nb_ports) < 0)
                rte_exit(EXIT_FAILURE, "check_port_config failed\n");

        nb_lcores = rte_lcore_count();

	/* We have to initialize all ports - create membufs, tx/rx queues, etc.*/
	for (portid = 0; portid < nb_ports; portid++) {
                /* skip ports that are not enabled */
                if ((enabled_port_mask & (1 << portid)) == 0) {
                        printf("\nSkipping disabled port %d\n", portid);
                        continue;
                }

                /* init port */
                printf("Initializing port %d ... ", portid );
                fflush(stdout);

                nb_rx_queue = get_port_n_rx_queues(portid);
                n_tx_queue = nb_lcores;
                if (n_tx_queue > MAX_TX_QUEUE_PER_PORT)
                        n_tx_queue = MAX_TX_QUEUE_PER_PORT;
                printf("Creating queues: nb_rxq=%d nb_txq=%u... ",
                        nb_rx_queue, (unsigned)n_tx_queue );
                ret = rte_eth_dev_configure(portid, nb_rx_queue,
                                        (uint16_t)n_tx_queue, &port_conf);
                if (ret < 0)
                        rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%d\n",
                                ret, portid);

                rte_eth_macaddr_get(portid, &ports_eth_addr[portid]);
/*                print_ethaddr(" Address:", &ports_eth_addr[portid]);
		printf("\n");*/

                printf("Port %u, MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
                                (unsigned) portid,
                                ports_eth_addr[portid].addr_bytes[0],
                                ports_eth_addr[portid].addr_bytes[1],
                                ports_eth_addr[portid].addr_bytes[2],
                                ports_eth_addr[portid].addr_bytes[3],
                                ports_eth_addr[portid].addr_bytes[4],
                                ports_eth_addr[portid].addr_bytes[5]);

                /* initialize port stats */
/*                memset(&port_statistics, 0, sizeof(port_statistics));*/

                /* init memory */
                ret = init_mem(NB_MBUF);
                if (ret < 0)
                        rte_exit(EXIT_FAILURE, "init_mem failed\n");

                /* init one TX queue per couple (lcore,port) */
                queueid = 0;
                for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
                        if (rte_lcore_is_enabled(lcore_id) == 0)
                                continue;

			socketid = get_socketid(lcore_id);

                        printf("txq=%u,%d,%d ", lcore_id, queueid, socketid);
                        fflush(stdout);

                        rte_eth_dev_info_get(portid, &dev_info);
                        txconf = &dev_info.default_txconf;
                        if (port_conf.rxmode.jumbo_frame)
                                txconf->txq_flags = 0;
                        ret = rte_eth_tx_queue_setup(portid, queueid, nb_txd,
                                                     socketid, txconf);
                        if (ret < 0)
                                rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup: err=%d, "
                                        "port=%d\n", ret, portid);

                        qconf = &lcore_conf[lcore_id];
                        qconf->tx_queue_id[portid] = queueid;
                        queueid++;
                }
		printf("\n");
	}

	memset(&port_statistics, 0, sizeof(port_statistics));

        for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
                if (rte_lcore_is_enabled(lcore_id) == 0)
                        continue;
                qconf = &lcore_conf[lcore_id];
                printf("\nInitializing RX queues on lcore %u ... ", lcore_id );
                fflush(stdout);
                /* init RX queues */
                for(queue = 0; queue < qconf->n_rx_queue; ++queue) {
                        portid = qconf->rx_queue_list[queue].port_id;
                        queueid = qconf->rx_queue_list[queue].queue_id;

			socketid = get_socketid(lcore_id);

                        printf("rxq=%d,%d,%d ", portid, queueid, socketid);
                        fflush(stdout);

                        ret = rte_eth_rx_queue_setup(portid, queueid, nb_rxd,
                                        socketid,
                                        NULL,
                                        pktmbuf_pool[socketid]);
                        if (ret < 0)
                                rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup: err=%d,"
                                                "port=%d\n", ret, portid);
                }
        }

        printf("\n");


        /* start ports */
        for (portid = 0; portid < nb_ports; portid++) {
                if ((enabled_port_mask & (1 << portid)) == 0) {
                        continue;
                }
                /* Start device */
                ret = rte_eth_dev_start(portid);
                if (ret < 0)
                        rte_exit(EXIT_FAILURE, "rte_eth_dev_start: err=%d, port=%d\n",
                                ret, portid);

		printf("Entering promiscous mode on port %d\n", portid);
                rte_eth_promiscuous_enable(portid);
        }


	check_all_ports_link_status(nb_ports, enabled_port_mask);

#endif

        init_memories();
        init_lcore_confs();

	return nb_ports;
}

