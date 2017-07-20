/* Copyright (c) 2016, Linaro Limited
 * All rights reserved.
 *
 * SPDX-License-Identifier:     BSD-3-Clause
 */

#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>

#include <odp_api.h>
#include <odp/helper/linux.h>
#include <odp/helper/eth.h>
#include <odp/helper/ip.h>

#include "includes/freescale/backend.h"
#include "includes/freescale/freescale_tables.h"
#include "includes/shared/dataplane.h"
#include "includes/compiled/actions.h"
//#include "includes/compiled/tables.c"

extern lookup_table_t table_config[];
extern counter_t counter_config[];
extern p4_register_t register_config[];
/** Maximum number of worker threads */
#define MAX_WORKERS            32

/** Size of the shared memory block */
#define SHM_PKT_POOL_SIZE      8192

/** Buffer size of the packet pool buffer */
#define SHM_PKT_POOL_BUF_SIZE  1856

/** Maximum number of packet in a burst */
#define MAX_PKT_BURST          32

/** Maximum number of pktio queues per interface */
#define MAX_QUEUES             32

/** Maximum number of pktio interfaces. Must be <= UINT8_MAX. */
#define MAX_PKTIOS             8

/** Number of MAC table entries. Must match to hash length. */
#define MAC_TBL_SIZE           UINT16_MAX

/** Get rid of path in filename - only for unix-type paths using '/' */
#define NO_PATH(file_name) (strrchr((file_name), '/') ? \
			    strrchr((file_name), '/') + 1 : (file_name))

/** Local copy of MAC forwarding table entry */
typedef union {
	struct {
		odph_ethaddr_t mac; /**< Ethernet MAC address */
		uint8_t port;	    /**< Port index */
	} s;

	uint64_t u64;
} mac_tbl_entry_t;

/**
 * Parsed command line application arguments
 */
typedef struct {
	int cpu_count;	   /**< Number of CPUs to be used */
	unsigned if_count; /**< Number of interfaces to be used */
	int num_workers;   /**< Number of worker threads */
	char **if_names;   /**< Array of pointers to interface names */
	int time;	   /**< Time in seconds to run */
	int accuracy;	   /**< Statistics get and print interval in seconds */
	char *if_str;	   /**< Storage for interface names */
} appl_args_t;

static int exit_threads;   /**< Break workers loop if set to 1 */

/**
 * Statistics
 */
typedef union {
	struct {
		/** Number of received packets */
		uint64_t rx_packets;
		/** Number of transmitted packets */
		uint64_t tx_packets;
		/** Packets dropped due to receive error */
		uint64_t rx_drops;
		/** Packets dropped due to transmit error */
		uint64_t tx_drops;
	} s;

	uint8_t padding[ODP_CACHE_LINE_SIZE];
} stats_t ODP_ALIGNED_CACHE;

/**
 * Packet buffer
 */
typedef struct pkt_buf_t {
	odp_packet_t pkt[MAX_PKT_BURST]; /**< Array of packet handles */
	unsigned len;			 /**< Number of packets in buffer */
} pkt_buf_t;

/**
 * Thread specific arguments
 */
typedef struct thread_args_t {
	 /** Number of interfaces from which to receive packets */
	int num_rx_pktio;
	struct {
		odp_pktin_queue_t pktin;   /**< Packet input queue */
		uint8_t port_idx;	   /**< Port index */
		int queue_idx;		   /**< Queue index */
	} rx_pktio[MAX_PKTIOS];
	struct {
		odp_pktout_queue_t pktout; /**< Packet output queue */
		int queue_idx;		   /**< Queue index */
		pkt_buf_t buf;		   /**< Packet TX buffer */
	} tx_pktio[MAX_PKTIOS];

	stats_t *stats[MAX_PKTIOS];	   /**< Interface statistics */
} thread_args_t;

/**
 * Grouping of all global data
 */
typedef struct {
	/** Per thread interface statistics */
	stats_t stats[MAX_WORKERS][MAX_PKTIOS];
	appl_args_t appl;		   /**< Parsed application arguments */
	thread_args_t thread[MAX_WORKERS]; /**< Thread specific arguments */
	odp_pool_t pool;		   /**< Packet pool */
	/** Table of pktio handles */
	struct {
		odp_pktio_t pktio;
		odp_pktin_queue_t pktin[MAX_QUEUES];
		odp_pktout_queue_t pktout[MAX_QUEUES];
		int num_rx_thr;
		int num_rx_queue;
		int num_tx_queue;
		int next_rx_queue;
		int next_tx_queue;
	} pktios[MAX_PKTIOS];

	odp_atomic_u64_t mac_tbl[MAC_TBL_SIZE]; /**< MAC forwarding table */
} args_t;

/** Global pointer to args */
static args_t *gbl_args;

/** Global barrier to synchronize main and workers */
static odp_barrier_t barrier;

/**
 * Calculate MAC table index using Ethernet address hash
 *
 * @param mac        Pointer to Ethernet address
 *
 * @retval MAC table index
 */
static inline uint16_t calc_mac_tbl_idx(odph_ethaddr_t *mac)
{
	uint32_t hash;

	hash = odp_hash_crc32c(mac->addr, ODPH_ETHADDR_LEN, 0);

	return (uint16_t)(hash & 0xFFFF);
}

static void
create_table(lookup_table_t* t)
{
    if(t->key_size == 0) return; // we don't create the table if there are no keys (it's a fake table for an element in the pipeline)
    switch(t->type)
    {
        case LOOKUP_EXACT:
            exact_create(t, 1);
            break;
        case LOOKUP_LPM:
            //lpm_create(t, 1);
            break;
        case LOOKUP_TERNARY:
            //ternary_create(t, 1);
            break;
    }
    debug("Created table %s on socket.\n", t->name);
}

static void
fill_table(lookup_table_t* t)
{
	uint8_t i;
	for(i=0; i<100;i++){
		struct action_forward_params param = {.port=1,}; 
		struct dmac_action action = {.action_id=2, action.forward_params=param,};
		uint8_t key[6] = {2,17,0,0,0,i};
		exact_add(t, key, (uint8_t*)&action);
	}
}

static void
create_tables(lookup_table_t** tables)
{
    if(table_config == NULL) return;
    debug("Initializing tables\n");
    int i;
    for(i = 0; i < NB_TABLES; i++) {
        tables[i] = &table_config[i];
	    create_table(tables[i]);
	    fill_table(tables[i]);
        debug("Creating instances for table %s\n", t.name);
    }
}


/**
 * Create a pktio handle
 *
 * @param dev        Name of device to open
 * @param index      Pktio index
 * @param num_rx     Number of RX queues
 * @param num_tx     Number of TX queues
 * @param pool       Pool to associate with device for packet RX/TX
 *
 * @retval 0 on success
 * @retval -1 on failure
 */
static int create_pktio(const char *dev, int idx, int num_rx, int num_tx,
			odp_pool_t pool)
{
	odp_pktio_t pktio;
	odp_pktio_param_t pktio_param;
	odp_pktio_capability_t capa;
	odp_pktin_queue_param_t pktin_param;
	odp_pktout_queue_param_t pktout_param;
	odp_pktio_op_mode_t mode_rx;
	odp_pktio_op_mode_t mode_tx;

	odp_pktio_param_init(&pktio_param);

	pktio = odp_pktio_open(dev, pool, &pktio_param);
	if (pktio == ODP_PKTIO_INVALID) {
		printf("Error: failed to open %s\n", dev);
		return -1;
	}

	printf("created pktio %" PRIu64 " (%s)\n", odp_pktio_to_u64(pktio),
	       dev);

	if (odp_pktio_capability(pktio, &capa)) {
		printf("Error: capability query failed %s\n", dev);
		return -1;
	}

	odp_pktin_queue_param_init(&pktin_param);
	odp_pktout_queue_param_init(&pktout_param);

	mode_tx = ODP_PKTIO_OP_MT_UNSAFE;
	mode_rx = ODP_PKTIO_OP_MT_UNSAFE;

	if (num_rx > (int)capa.max_input_queues) {
		printf("Sharing %i input queues between %i workers\n",
		       capa.max_input_queues, num_rx);
		num_rx  = capa.max_input_queues;
		mode_rx = ODP_PKTIO_OP_MT;
	}

	if (num_tx > (int)capa.max_output_queues) {
		printf("Sharing %i output queues between %i workers\n",
		       capa.max_output_queues, num_tx);
		num_tx  = capa.max_output_queues;
		mode_tx = ODP_PKTIO_OP_MT;
	}

	pktin_param.hash_enable = 1;
	pktin_param.hash_proto.proto.ipv4 = 1;
	pktin_param.hash_proto.proto.ipv4_tcp = 1;
	pktin_param.hash_proto.proto.ipv4_udp = 1;
	pktin_param.num_queues  = num_rx;
	pktin_param.op_mode     = mode_rx;

	pktout_param.op_mode    = mode_tx;
	pktout_param.num_queues = num_tx;

	if (odp_pktin_queue_config(pktio, &pktin_param)) {
		printf("Error: input queue config failed %s\n", dev);
		return -1;
	}
	if (odp_pktout_queue_config(pktio, &pktout_param)) {
		printf("Error: output queue config failed %s\n", dev);
		return -1;
	}
	if (odp_pktin_queue(pktio, gbl_args->pktios[idx].pktin,
			    num_rx) != num_rx) {
		printf("Error: pktin queue query failed %s\n", dev);
		return -1;
	}
	if (odp_pktout_queue(pktio, gbl_args->pktios[idx].pktout,
			     num_tx) != num_tx) {
		printf("Error: pktout queue query failed %s\n", dev);
		return -1;
	}

	printf("created %i input and %i output queues on (%s)\n", num_rx,
	       num_tx, dev);

	gbl_args->pktios[idx].num_rx_queue = num_rx;
	gbl_args->pktios[idx].num_tx_queue = num_tx;
	gbl_args->pktios[idx].pktio        = pktio;

	return 0;
}

/**
 *  Print statistics
 *
 * @param num_workers Number of worker threads
 * @param thr_stats   Pointer to stats storage
 * @param duration    Number of seconds to loop in
 * @param timeout     Number of seconds for stats calculation
 *
 * @retval 0 on success
 * @retval -1 on failure
 */
static int print_speed_stats(int num_workers, stats_t (*thr_stats)[MAX_PKTIOS],
			     int duration, int timeout)
{
	uint64_t rx_pkts_prev[MAX_PKTIOS] = {0};
	uint64_t tx_pkts_prev[MAX_PKTIOS] = {0};
	uint64_t rx_pkts_tot;
	uint64_t tx_pkts_tot;
	uint64_t rx_pps;
	uint64_t tx_pps;
	int i, j;
	int elapsed = 0;
	int stats_enabled = 1;
	int loop_forever = (duration == 0);
	int num_ifaces = gbl_args->appl.if_count;

	if (timeout <= 0) {
		stats_enabled = 0;
		timeout = 1;
	}
	/* Wait for all threads to be ready*/
	odp_barrier_wait(&barrier);

	do {
		uint64_t rx_pkts[MAX_PKTIOS] = {0};
		uint64_t tx_pkts[MAX_PKTIOS] = {0};
		uint64_t rx_drops = 0;
		uint64_t tx_drops = 0;

		rx_pkts_tot = 0;
		tx_pkts_tot = 0;

		sleep(timeout);
		elapsed += timeout;

		for (i = 0; i < num_workers; i++) {
			for (j = 0; j < num_ifaces; j++) {
				rx_pkts[j] += thr_stats[i][j].s.rx_packets;
				tx_pkts[j] += thr_stats[i][j].s.tx_packets;
				rx_drops += thr_stats[i][j].s.rx_drops;
				tx_drops += thr_stats[i][j].s.tx_drops;
			}
		}

		if (!stats_enabled)
			continue;

		for (j = 0; j < num_ifaces; j++) {
			rx_pps = (rx_pkts[j] - rx_pkts_prev[j]) / timeout;
			tx_pps = (tx_pkts[j] - tx_pkts_prev[j]) / timeout;
			printf("  Port %d: %" PRIu64 " rx pps, %" PRIu64
			       " tx pps, %" PRIu64 " rx pkts, %" PRIu64
			       " tx pkts\n", j, rx_pps, tx_pps, rx_pkts[j],
			       tx_pkts[j]);

			rx_pkts_prev[j] = rx_pkts[j];
			tx_pkts_prev[j] = tx_pkts[j];
			rx_pkts_tot += rx_pkts[j];
			tx_pkts_tot += tx_pkts[j];
		}

		printf("Total: %" PRIu64 " rx pkts, %" PRIu64 " tx pkts, %"
		       PRIu64 " rx drops, %" PRIu64 " tx drops\n", rx_pkts_tot,
		       tx_pkts_tot, rx_drops, tx_drops);

	} while (loop_forever || (elapsed < duration));

	return rx_pkts_tot >= 100 ? 0 : -1;
}

/**
 *  Print switch worker mappings and port configuration
 */
static void print_port_mapping(void)
{
	int if_count, num_workers;
	int thr, pktio;

	if_count    = gbl_args->appl.if_count;
	num_workers = gbl_args->appl.num_workers;

	printf("\nWorker mapping table (port[queue])\n--------------------\n");

	for (thr = 0; thr < num_workers; thr++) {
		uint8_t port_idx;
		int queue_idx;
		thread_args_t *thr_args = &gbl_args->thread[thr];
		int num = thr_args->num_rx_pktio;

		printf("Worker %i\n", thr);

		for (pktio = 0; pktio < num; pktio++) {
			port_idx = thr_args->rx_pktio[pktio].port_idx;
			queue_idx =  thr_args->rx_pktio[pktio].queue_idx;
			printf("  %i[%i]\n", port_idx, queue_idx);
		}
	}

	printf("\nPort config\n--------------------\n");

	for (pktio = 0; pktio < if_count; pktio++) {
		const char *dev = gbl_args->appl.if_names[pktio];

		printf("Port %i (%s)\n", pktio, dev);
		printf("  rx workers %i\n",
		       gbl_args->pktios[pktio].num_rx_thr);
		printf("  rx queues %i\n",
		       gbl_args->pktios[pktio].num_rx_queue);
		printf("  tx queues %i\n",
		       gbl_args->pktios[pktio].num_tx_queue);
	}

	printf("\n");
}
 
static inline void forward_packets(odp_packet_t pkt_tbl[], unsigned num,
				   thread_args_t *thr_arg, uint8_t port_in, packet_descriptor_t *pd, lookup_table_t** tables)
{
	odp_packet_t pkt;
	odph_ethhdr_t *eth;
	unsigned i;
	unsigned buf_id;
	int ret;
	uint8_t port_out = 0;
	//uint8_t counter = 1;
	for (i = 0; i < num; i++) {
		pkt = pkt_tbl[i];
		pd->data=odp_packet_data(pkt);
		handle_packet(pd, tables);
		EXTRACT_INT32_BITS(pd, field_instance_standard_metadata_egress_port, port_out);
		//port_out=(port_in+1)%2;
		buf_id = thr_arg->tx_pktio[port_out].buf.len;
		thr_arg->tx_pktio[port_out].buf.pkt[buf_id] = pkt;
		thr_arg->tx_pktio[port_out].buf.len++;
	}
}

/*
 * Bind worker threads to switch ports and calculate number of queues needed
 *
 * less workers (N) than interfaces (M)
 *  - assign each worker to process every Nth interface
 *  - workers process inequal number of interfaces, when M is not divisible by N
 *  - needs only single queue per interface
 * otherwise
 *  - assign an interface to every Mth worker
 *  - interfaces are processed by inequal number of workers, when N is not
 *    divisible by M
 *  - tries to configure a queue per worker per interface
 *  - shares queues, if interface capability does not allows a queue per worker
 */
static void bind_workers(void)
{
	int if_count, num_workers;
	int rx_idx, thr, pktio;
	thread_args_t *thr_args;

	if_count    = gbl_args->appl.if_count;
	num_workers = gbl_args->appl.num_workers;

	if (if_count > num_workers) {
		thr = 0;

		for (rx_idx = 0; rx_idx < if_count; rx_idx++) {
			thr_args = &gbl_args->thread[thr];
			pktio    = thr_args->num_rx_pktio;
			thr_args->rx_pktio[pktio].port_idx = rx_idx;
			thr_args->num_rx_pktio++;

			gbl_args->pktios[rx_idx].num_rx_thr++;

			thr++;
			if (thr >= num_workers)
				thr = 0;
		}
	} else {
		rx_idx = 0;

		for (thr = 0; thr < num_workers; thr++) {
			thr_args = &gbl_args->thread[thr];
			pktio    = thr_args->num_rx_pktio;
			thr_args->rx_pktio[pktio].port_idx = rx_idx;
			thr_args->num_rx_pktio++;

			gbl_args->pktios[rx_idx].num_rx_thr++;

			rx_idx++;
			if (rx_idx >= if_count)
				rx_idx = 0;
		}
	}
}

/**
 * Switch worker thread
 *
 * @param arg  Thread arguments of type 'thread_args_t *'
 */
static int run_worker(void *arg)
{
	packet_descriptor_t* pd = malloc(sizeof(packet_descriptor_t));
	lookup_table_t** tables = malloc(2*sizeof(lookup_table_t));
	init_dataplane(pd, tables);
	create_tables(tables);
	
	thread_args_t *thr_args = arg;
	odp_packet_t pkt_tbl[MAX_PKT_BURST];
	odp_pktin_queue_t pktin;
	odp_pktout_queue_t pktout;
	unsigned num_pktio;
	unsigned pktio = 0;
	uint8_t port_in;
	uint8_t port_out;
	int pkts;

	num_pktio = thr_args->num_rx_pktio;
	pktin     = thr_args->rx_pktio[pktio].pktin;
	port_in  = thr_args->rx_pktio[pktio].port_idx;
	
	odp_barrier_wait(&barrier);

	while (!exit_threads) {
		int sent;
		unsigned drops;

		if (num_pktio > 1) {
			pktin     = thr_args->rx_pktio[pktio].pktin;
			port_in = thr_args->rx_pktio[pktio].port_idx;
			pktio++;
			if (pktio == num_pktio)
				pktio = 0;
		}

		pkts = odp_pktin_recv(pktin, pkt_tbl, MAX_PKT_BURST);
		if (odp_unlikely(pkts <= 0))
			continue;

		thr_args->stats[port_in]->s.rx_packets += pkts;

		/* Sort packets to thread local tx buffers */
		forward_packets(pkt_tbl, pkts, thr_args, port_in, pd, tables);

		/* Empty all thread local tx buffers */
		for (port_out = 0; port_out < gbl_args->appl.if_count;
				port_out++) {
			unsigned tx_pkts;
			odp_packet_t *tx_pkt_tbl;

			if (port_out == port_in ||
			    thr_args->tx_pktio[port_out].buf.len == 0)
				continue;
            
			tx_pkts = thr_args->tx_pktio[port_out].buf.len;
			thr_args->tx_pktio[port_out].buf.len = 0;

			tx_pkt_tbl = thr_args->tx_pktio[port_out].buf.pkt;

			pktout = thr_args->tx_pktio[port_out].pktout;

			sent = odp_pktout_send(pktout, tx_pkt_tbl, tx_pkts);
			sent = odp_unlikely(sent < 0) ? 0 : sent;

			thr_args->stats[port_out]->s.tx_packets += sent;

			drops = tx_pkts - sent;

			if (odp_unlikely(drops)) {
				unsigned i;

				thr_args->stats[port_out]->s.tx_drops += drops;

				/* Drop rejected packets */
				for (i = sent; i < tx_pkts; i++)
					odp_packet_free(tx_pkt_tbl[i]);
			}
		}
	}

	/* Make sure that latest stat writes are visible to other threads */
	odp_mb_full();

	return 0;
}

/*
 * Bind queues to threads and fill in missing thread arguments (handles)
 */
static void bind_queues(void)
{
	int num_workers;
	int thr, pktio;

	num_workers = gbl_args->appl.num_workers;

	for (thr = 0; thr < num_workers; thr++) {
		int rx_idx;
		thread_args_t *thr_args = &gbl_args->thread[thr];
		int num = thr_args->num_rx_pktio;

		/* Receive only from selected ports */
		for (pktio = 0; pktio < num; pktio++) {
			int rx_queue;

			rx_idx   = thr_args->rx_pktio[pktio].port_idx;
			rx_queue = gbl_args->pktios[rx_idx].next_rx_queue;

			thr_args->rx_pktio[pktio].pktin =
				gbl_args->pktios[rx_idx].pktin[rx_queue];
			thr_args->rx_pktio[pktio].queue_idx = rx_queue;

			rx_queue++;
			if (rx_queue >= gbl_args->pktios[rx_idx].num_rx_queue)
				rx_queue = 0;
			gbl_args->pktios[rx_idx].next_rx_queue = rx_queue;
		}
		/* Send to all ports */
		for (pktio = 0; pktio < (int)gbl_args->appl.if_count; pktio++) {
			int tx_queue;

			tx_queue = gbl_args->pktios[pktio].next_tx_queue;

			thr_args->tx_pktio[pktio].pktout =
				gbl_args->pktios[pktio].pktout[tx_queue];
			thr_args->tx_pktio[pktio].queue_idx = tx_queue;

			tx_queue++;
			if (tx_queue >= gbl_args->pktios[pktio].num_tx_queue)
				tx_queue = 0;
			gbl_args->pktios[pktio].next_tx_queue = tx_queue;
		}
	}
}

/**
 * Print usage information
 */
static void usage(char *progname)
{
	printf("\n"
	       "OpenDataPlane learning switch example.\n"
	       "\n"
	       "Usage: %s OPTIONS\n"
	       "  E.g. %s -i eth0,eth1,eth2,eth3\n"
	       "\n"
	       "Mandatory OPTIONS:\n"
	       "  -i, --interface Eth interfaces (comma-separated, no spaces)\n"
	       "                  Interface count min 2, max %i\n"
	       "\n"
	       "Optional OPTIONS:\n"
	       "  -c, --count <number> CPU count.\n"
	       "  -t, --time  <number> Time in seconds to run.\n"
	       "  -a, --accuracy <number> Statistics print interval in seconds\n"
	       "                          (default is 10 second).\n"
	       "  -h, --help           Display help and exit.\n\n"
	       "\n", NO_PATH(progname), NO_PATH(progname), MAX_PKTIOS
	    );
}

/**
 * Parse and store the command line arguments
 *
 * @param argc       argument count
 * @param argv[]     argument vector
 * @param appl_args  Store application arguments here
 */
static void parse_args(int argc, char *argv[], appl_args_t *appl_args)
{
	int opt;
	int long_index;
	char *token;
	size_t len;
	unsigned i;
	static const struct option longopts[] = {
		{"count", required_argument, NULL, 'c'},
		{"time", required_argument, NULL, 't'},
		{"accuracy", required_argument, NULL, 'a'},
		{"interface", required_argument, NULL, 'i'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}
	};

	static const char *shortopts = "+c:+t:+a:i:h";

	/* let helper collect its own arguments (e.g. --odph_proc) */
	odph_parse_options(argc, argv, shortopts, longopts);

	appl_args->time = 0; /* loop forever if time to run is 0 */
	appl_args->accuracy = 10; /* get and print pps stats second */

	opterr = 0; /* do not issue errors on helper options */

	while (1) {
		opt = getopt_long(argc, argv, shortopts, longopts, &long_index);

		if (opt == -1)
			break;	/* No more options */

		switch (opt) {
		case 'c':
			appl_args->cpu_count = atoi(optarg);
			break;
		case 't':
			appl_args->time = atoi(optarg);
			break;
		case 'a':
			appl_args->accuracy = atoi(optarg);
			break;
		case 'i':
			len = strlen(optarg);
			if (len == 0) {
				usage(argv[0]);
				exit(EXIT_FAILURE);
			}
			len += 1;	/* add room for '\0' */

			appl_args->if_str = malloc(len);
			if (appl_args->if_str == NULL) {
				usage(argv[0]);
				exit(EXIT_FAILURE);
			}

			/* count the number of tokens separated by ',' */
			strcpy(appl_args->if_str, optarg);
			for (token = strtok(appl_args->if_str, ","), i = 0;
			     token != NULL;
			     token = strtok(NULL, ","), i++)
				;

			appl_args->if_count = i;

			if (appl_args->if_count < 2 ||
			    appl_args->if_count > MAX_PKTIOS) {
				usage(argv[0]);
				exit(EXIT_FAILURE);
			}

			/* allocate storage for the if names */
			appl_args->if_names =
			    calloc(appl_args->if_count, sizeof(char *));

			/* store the if names (reset names string) */
			strcpy(appl_args->if_str, optarg);
			for (token = strtok(appl_args->if_str, ","), i = 0;
			     token != NULL; token = strtok(NULL, ","), i++) {
				appl_args->if_names[i] = token;
			}
			break;
		case 'h':
			usage(argv[0]);
			exit(EXIT_SUCCESS);
			break;
		default:
			break;
		}
	}

	if (appl_args->if_count == 0) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	optind = 1;		/* reset 'extern optind' from the getopt lib */
}

/**
 * Print system and application info
 */
static void print_info(char *progname, appl_args_t *appl_args)
{
	unsigned i;

	printf("\n"
	       "ODP system info\n"
	       "---------------\n"
	       "ODP API version: %s\n"
	       "ODP impl name:   %s\n"
	       "CPU model:       %s\n"
	       "CPU freq (hz):   %" PRIu64 "\n"
	       "Cache line size: %i\n"
	       "CPU count:       %i\n"
	       "\n",
	       odp_version_api_str(), odp_version_impl_name(),
	       odp_cpu_model_str(), odp_cpu_hz_max(),
	       odp_sys_cache_line_size(), odp_cpu_count());

	printf("Running ODP appl: \"%s\"\n"
	       "-----------------\n"
	       "IF-count:        %i\n"
	       "Using IFs:      ",
	       progname, appl_args->if_count);
	for (i = 0; i < appl_args->if_count; ++i)
		printf(" %s", appl_args->if_names[i]);
	printf("\n\n");
	fflush(NULL);
}

static void gbl_args_init(args_t *args)
{
	int pktio;

	memset(args, 0, sizeof(args_t));

	for (pktio = 0; pktio < MAX_PKTIOS; pktio++)
		args->pktios[pktio].pktio = ODP_PKTIO_INVALID;
}

int main(int argc, char **argv)
{
	odph_odpthread_t thread_tbl[MAX_WORKERS];
	int i, j;
	int cpu;
	int num_workers;
	odp_shm_t shm;
	odp_cpumask_t cpumask;
	char cpumaskstr[ODP_CPUMASK_STR_SIZE];
	odp_pool_param_t params;
	int ret;
	stats_t (*stats)[MAX_PKTIOS];
	int if_count;
	odp_instance_t instance;
	odph_odpthread_params_t thr_params;

	/* Init ODP before calling anything else */
	if (odp_init_global(&instance, NULL, NULL)) {
		printf("Error: ODP global init failed.\n");
		exit(EXIT_FAILURE);
	}

	/* Init this thread */
	if (odp_init_local(instance, ODP_THREAD_CONTROL)) {
		printf("Error: ODP local init failed.\n");
		exit(EXIT_FAILURE);
	}

	/* Reserve memory for args from shared mem */
	shm = odp_shm_reserve("shm_args", sizeof(args_t),
			      ODP_CACHE_LINE_SIZE, 0);
	gbl_args = odp_shm_addr(shm);

	if (gbl_args == NULL) {
		printf("Error: shared mem alloc failed.\n");
		exit(EXIT_FAILURE);
	}
	gbl_args_init(gbl_args);

	for (i = 0; (unsigned)i < MAC_TBL_SIZE; i++)
		odp_atomic_init_u64(&gbl_args->mac_tbl[i], 0);

	/* Parse and store the application arguments */
	parse_args(argc, argv, &gbl_args->appl);

	/* Print both system and application information */
	print_info(NO_PATH(argv[0]), &gbl_args->appl);

	/* Default to system CPU count unless user specified */
	num_workers = MAX_WORKERS;
	if (gbl_args->appl.cpu_count)
		num_workers = gbl_args->appl.cpu_count;

	/* Get default worker cpumask */
	num_workers = odp_cpumask_default_worker(&cpumask, num_workers);
	(void)odp_cpumask_to_str(&cpumask, cpumaskstr, sizeof(cpumaskstr));

	gbl_args->appl.num_workers = num_workers;

	if_count = gbl_args->appl.if_count;

	printf("num worker threads: %i\n", num_workers);
	printf("first CPU:          %i\n", odp_cpumask_first(&cpumask));
	printf("cpu mask:           %s\n", cpumaskstr);

	/* Create packet pool */
	odp_pool_param_init(&params);
	params.pkt.seg_len = SHM_PKT_POOL_BUF_SIZE;
	params.pkt.len     = SHM_PKT_POOL_BUF_SIZE;
	params.pkt.num     = SHM_PKT_POOL_SIZE;
	params.type        = ODP_POOL_PACKET;

	gbl_args->pool = odp_pool_create("packet pool", &params);
	if (gbl_args->pool == ODP_POOL_INVALID) {
		printf("Error: packet pool create failed.\n");
		exit(EXIT_FAILURE);
	}
	odp_pool_print(gbl_args->pool);

	bind_workers();

	for (i = 0; i < if_count; ++i) {
		const char *dev = gbl_args->appl.if_names[i];
		int num_rx;

		/* An RX queue per assigned worker and a private TX queue for
		 * each worker */
		num_rx = gbl_args->pktios[i].num_rx_thr;

		if (create_pktio(dev, i, num_rx, num_workers, gbl_args->pool))
			exit(EXIT_FAILURE);

		ret = odp_pktio_promisc_mode_set(gbl_args->pktios[i].pktio, 1);
		if (ret != 0) {
			printf("Error: failed to set port to promiscuous mode.\n");
			exit(EXIT_FAILURE);
		}
	}
	gbl_args->pktios[i].pktio = ODP_PKTIO_INVALID;

	bind_queues();

	print_port_mapping();

	memset(thread_tbl, 0, sizeof(thread_tbl));

	odp_barrier_init(&barrier, num_workers + 1);

	stats = gbl_args->stats;

	memset(&thr_params, 0, sizeof(thr_params));
	thr_params.thr_type = ODP_THREAD_WORKER;
	thr_params.instance = instance;
	thr_params.start    = run_worker;

	/* Create worker threads */
	cpu = odp_cpumask_first(&cpumask);
	for (i = 0; i < num_workers; ++i) {
		odp_cpumask_t thd_mask;

		for (j = 0; j < MAX_PKTIOS; j++)
			gbl_args->thread[i].stats[j] = &stats[i][j];

		thr_params.arg      = &gbl_args->thread[i];

		odp_cpumask_zero(&thd_mask);
		odp_cpumask_set(&thd_mask, cpu);
		odph_odpthreads_create(&thread_tbl[i], &thd_mask, &thr_params);
		cpu = odp_cpumask_next(&cpumask, cpu);
	}

	/* Start packet receive and transmit */
	for (i = 0; i < if_count; ++i) {
		odp_pktio_t pktio;

		pktio = gbl_args->pktios[i].pktio;
		ret   = odp_pktio_start(pktio);
		if (ret) {
			printf("Error: unable to start %s\n",
			       gbl_args->appl.if_names[i]);
			exit(EXIT_FAILURE);
		}
	}

	ret = print_speed_stats(num_workers, gbl_args->stats,
				gbl_args->appl.time, gbl_args->appl.accuracy);
	exit_threads = 1;

	/* Master thread waits for other threads to exit */
	for (i = 0; i < num_workers; ++i)
		odph_odpthreads_join(&thread_tbl[i]);

	free(gbl_args->appl.if_names);
	free(gbl_args->appl.if_str);

	if (odp_pool_destroy(gbl_args->pool)) {
		printf("Error: pool destroy\n");
		exit(EXIT_FAILURE);
	}

	if (odp_term_local()) {
		printf("Error: term local\n");
		exit(EXIT_FAILURE);
	}

	if (odp_term_global(instance)) {
		printf("Error: term global\n");
		exit(EXIT_FAILURE);
	}

	printf("Exit: %d\n\n", ret);
	return ret;
}
