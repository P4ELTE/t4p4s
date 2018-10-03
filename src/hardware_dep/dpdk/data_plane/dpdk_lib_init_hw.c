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


//=============================================================================
// Shared

extern struct socket_state state[NB_SOCKETS];

struct lcore_conf lcore_conf[RTE_MAX_LCORE];


uint16_t enabled_port_mask;

// NUMA is enabled by default.
int numa_on = 1;


uint16_t            nb_lcore_params;
struct lcore_params lcore_params[MAX_LCORE_PARAMS];

//=============================================================================
// Locals

struct rte_mempool* pktmbuf_pool[NB_SOCKETS];

struct ether_addr ports_eth_addr[RTE_MAX_ETHPORTS];

uint16_t t4p4s_nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
uint16_t t4p4s_nb_txd = RTE_TEST_TX_DESC_DEFAULT;


struct rte_eth_conf port_conf = {
    .rxmode = {
        .mq_mode = ETH_MQ_RX_RSS,
        .max_rx_pkt_len = ETHER_MAX_LEN,
        .split_hdr_size = 0,
        /*
        .header_split   = 0, // Header Split disabled
        .hw_ip_checksum = 1, // IP checksum offload enabled
        .hw_vlan_filter = 0, // VLAN filtering disabled
        .jumbo_frame    = 0, // Jumbo Frame Support disabled
        .hw_strip_crc   = 1, // CRC stripped by hardware
        */
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

int check_lcore_params()
{
    for (uint16_t i = 0; i < nb_lcore_params; ++i) {
        uint8_t queue = lcore_params[i].queue_id;

        if (queue >= MAX_RX_QUEUE_PER_PORT) {
            printf("queue number %hhu over maximum receiving queues per port (%hhu)\n", queue, MAX_RX_QUEUE_PER_PORT);
            return -1;
        }

        uint8_t lcore = lcore_params[i].lcore_id;

        if (!rte_lcore_is_enabled(lcore)) {
            printf("error: lcore %hhu is not enabled in lcore mask\n", lcore);
            return -1;
        }

        int socketid = rte_lcore_to_socket_id(lcore);
        if (socketid != 0 && numa_on == 0) {
            printf("warning: lcore %hhu is on socket %d with numa off \n", lcore, socketid);
        }
    }
    return 0;
}

int check_port_config(unsigned nb_ports)
{
    for (uint16_t i = 0; i < nb_lcore_params; ++i) {
        unsigned portid = lcore_params[i].port_id;

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

//=============================================================================

int init_lcore_rx_queues()
{
    for (uint16_t i = 0; i < nb_lcore_params; ++i) {
        uint8_t  lcore       = lcore_params[i].lcore_id;
        uint16_t nb_rx_queue = lcore_conf[lcore].hw.n_rx_queue;

        if (nb_rx_queue >= MAX_RX_QUEUE_PER_LCORE) {
            printf("error: too many queues (%u) for lcore %u (max: %u)\n",
                (unsigned)nb_rx_queue + 1, (unsigned)lcore, MAX_RX_QUEUE_PER_LCORE);
            return -1;
        }

        lcore_conf[lcore].hw.rx_queue_list[nb_rx_queue].port_id  = lcore_params[i].port_id;
        lcore_conf[lcore].hw.rx_queue_list[nb_rx_queue].queue_id = lcore_params[i].queue_id;
        lcore_conf[lcore].hw.n_rx_queue++;
    }
    return 0;
}

int init_lcore_confs()
{
    debug(" :::: Configuring lcore structs...\n");

    for (unsigned lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
        if (rte_lcore_is_enabled(lcore_id) == 0) continue;

        int socketid = get_socketid(lcore_id);

        struct lcore_conf* qconf = &lcore_conf[lcore_id];

        for(int i = 0; i < NB_TABLES; i++)
            qconf->state.tables[i] = state[socketid].tables[i][0];
    }
    return 0;
}


void init_mbuf_pool(unsigned lcore_id)
{

    printf("mpb0\n");
    if (rte_lcore_is_enabled(lcore_id) == 0)   return;

    printf("mpb\n");

    int socketid = get_socketid(lcore_id);

    if (pktmbuf_pool[socketid] != NULL)   return;

    char s[64];
    snprintf(s, sizeof(s), "mbuf_pool_%d", socketid);
    pktmbuf_pool[socketid] =
        rte_mempool_create(s, NB_MBUF, MBUF_SIZE, MEMPOOL_CACHE_SIZE,
            sizeof(struct rte_pktmbuf_pool_private),
            rte_pktmbuf_pool_init, NULL,
            rte_pktmbuf_init, NULL,
            socketid, 0);

    if (pktmbuf_pool[socketid] == NULL)
        rte_exit(EXIT_FAILURE, "Cannot init mbuf pool on socket %d\n", socketid);

    debug("Allocated mbuf pool on socket %d\n", socketid);
}

uint32_t max(uint32_t val1, uint32_t val2) {
    return val1 > val2 ? val1 : val2;
}

void init_tx_on_lcore(unsigned lcore_id, uint8_t portid, uint16_t queueid)
{
    if (rte_lcore_is_enabled(lcore_id) == 0)
        return;

    uint8_t socketid = get_socketid(lcore_id);

    printf("txq=%u,%d,%d\n", lcore_id, queueid, socketid);
    fflush(stdout);

    struct rte_eth_dev_info dev_info;
    rte_eth_dev_info_get(portid, &dev_info);
    struct rte_eth_txconf* txconf = &dev_info.default_txconf;

    int ret = rte_eth_tx_queue_setup(portid, queueid, t4p4s_nb_txd, socketid, txconf);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup: err=%d, "
                 "port=%d\n", ret, portid);

    struct lcore_conf* qconf = &lcore_conf[lcore_id];
    qconf->hw.tx_queue_id[portid] = queueid;
}

// We have to initialize all ports - create membufs, tx/rx queues, etc.
void dpdk_init_port(uint8_t nb_ports, uint32_t nb_lcores, uint8_t portid) {
    /* skip ports that are not enabled */
    if ((enabled_port_mask & (1 << portid)) == 0) {
        printf("\nSkipping disabled port %d\n", portid);
        return;
    }

    printf("Initializing port %d ...\n", portid);
    fflush(stdout);

    uint16_t nb_rx_queue = get_port_n_rx_queues(portid);
    uint32_t n_tx_queue = max(nb_lcores, MAX_TX_QUEUE_PER_PORT);

    printf("Creating queues: nb_rxq=%d nb_txq=%u...\n",
          nb_rx_queue, (unsigned)n_tx_queue );
    int ret = rte_eth_dev_configure(portid, nb_rx_queue,
                                (uint16_t)n_tx_queue, &port_conf);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%d\n",
                 ret, portid);

    rte_eth_macaddr_get(portid, &ports_eth_addr[portid]);
    print_port_mac((unsigned)portid, ports_eth_addr[portid].addr_bytes);

    for (unsigned lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
        init_mbuf_pool(lcore_id);
    }

    uint16_t queueid = 0;
    for (unsigned lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++, queueid++) {
        init_tx_on_lcore(lcore_id, portid, queueid);
    }

    printf("\n");
}

void dpdk_init_rx_queue(uint8_t queue, unsigned lcore_id, struct lcore_conf* qconf) {
    uint8_t portid = qconf->hw.rx_queue_list[queue].port_id;
    uint16_t queueid = qconf->hw.rx_queue_list[queue].queue_id;

    int socketid = get_socketid(lcore_id);

    printf("rxq=%d,%d,%d ", portid, queueid, socketid);
    fflush(stdout);

    int ret = rte_eth_rx_queue_setup(portid, queueid, t4p4s_nb_rxd,
                                 socketid, NULL, pktmbuf_pool[socketid]);
    if (ret < 0)
        rte_exit(EXIT_FAILURE,
                 "rte_eth_rx_queue_setup: err=%d,port=%d\n", ret, portid);
}

void dpdk_init_lcore(unsigned lcore_id) {
    if (rte_lcore_is_enabled(lcore_id) == 0)
        return;

    struct lcore_conf* qconf = &lcore_conf[lcore_id];
    printf("\nInitializing RX queues on lcore %u ... ", lcore_id );
    fflush(stdout);

    /* init RX queues */
    for (uint8_t queue = 0; queue < qconf->hw.n_rx_queue; ++queue) {
        dpdk_init_rx_queue(queue, lcore_id, qconf);
    }
}

void dpdk_init_nic()
{
    int ret = init_lcore_rx_queues();
    if (ret < 0)
            rte_exit(EXIT_FAILURE, "init_lcore_rx_queues failed\n");

#if RTE_VERSION >= RTE_VERSION_NUM(18,05,0,0)
    uint16_t nb_ports = rte_eth_dev_count_avail();
#else
    // deprecated since DPDK 18.05
    uint16_t nb_ports = rte_eth_dev_count();
#endif

    if (nb_ports > RTE_MAX_ETHPORTS)
        nb_ports = RTE_MAX_ETHPORTS;

    if (check_port_config(nb_ports) < 0)
        rte_exit(EXIT_FAILURE, "check_port_config failed\n");

    uint32_t nb_lcores = rte_lcore_count();

    for (uint16_t portid = 0; portid < nb_ports; portid++) {
        dpdk_init_port(nb_ports, nb_lcores, portid);
    }

    for (unsigned lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
        dpdk_init_lcore(lcore_id);
    }

    printf("\n");

    /* start ports */
    for (uint16_t portid = 0; portid < nb_ports; portid++) {
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

    print_all_ports_link_status(nb_ports, enabled_port_mask);
}
