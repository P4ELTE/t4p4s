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

#include <rte_ethdev.h>

#include "dpdk_nicon.h"

extern int get_socketid(unsigned lcore_id);

extern struct lcore_conf lcore_conf[RTE_MAX_LCORE];
extern void dpdk_init_nic();
extern uint8_t get_nb_ports();

// ------------------------------------------------------
// Locals

struct rte_mempool *header_pool, *clone_pool;
extern struct rte_mempool* pktmbuf_pool[NB_SOCKETS];

struct rte_mbuf* deparse_mbuf;

// ------------------------------------------------------

/* Send burst of packets on an output interface */
static inline void send_burst(struct lcore_conf *conf, uint16_t n, uint8_t port)
{
    uint16_t queueid = conf->hw.tx_queue_id[port];
    struct rte_mbuf **m_table = (struct rte_mbuf **)conf->hw.tx_mbufs[port].m_table;

    int ret = rte_eth_tx_burst(port, queueid, m_table, n);
    if (unlikely(ret < n)) {
        do {
            rte_pktmbuf_free(m_table[ret]);
        } while (++ret < n);
    }
}

void tx_burst_queue_drain(struct lcore_data* lcdata) {
    uint64_t cur_tsc = rte_rdtsc();

    uint64_t diff_tsc = cur_tsc - lcdata->prev_tsc;
    if (unlikely(diff_tsc > lcdata->drain_tsc)) {
        for (unsigned portid = 0; portid < get_nb_ports(); portid++) {
            if (lcdata->conf->hw.tx_mbufs[portid].len == 0)
                continue;

            send_burst(lcdata->conf,
                       lcdata->conf->hw.tx_mbufs[portid].len,
                       (uint8_t) portid);
            lcdata->conf->hw.tx_mbufs[portid].len = 0;
        }

        lcdata->prev_tsc = cur_tsc;
    }
}

// ------------------------------------------------------

static uint16_t
add_packet_to_queue(struct rte_mbuf *mbuf, uint8_t port, uint32_t lcore_id)
{
    struct lcore_conf *conf = &lcore_conf[lcore_id];
    uint16_t queue_length = conf->hw.tx_mbufs[port].len;
    conf->hw.tx_mbufs[port].m_table[queue_length] = mbuf;
    queue_length++;

    return queue_length;
}


/* creating replicas of a packet for  */
static inline struct rte_mbuf *
mcast_out_pkt(struct rte_mbuf *pkt, int use_clone)
{
    struct rte_mbuf *hdr;

    debug("mcast_out_pkt new mbuf is needed...\n");
        /* Create new mbuf for the header. */
        if ((hdr = rte_pktmbuf_alloc(header_pool)) == NULL)
                return (NULL);

    debug("hdr is allocated\n");

    /* If requested, then make a new clone packet. */
    if (use_clone != 0 &&
        (pkt = rte_pktmbuf_clone(pkt, clone_pool)) == NULL) {
            rte_pktmbuf_free(hdr);
            return (NULL);
    }

    debug("setup ne header\n");

    /* prepend new header */
    hdr->next = pkt;


    /* update header's fields */
    hdr->pkt_len = (uint16_t)(hdr->data_len + pkt->pkt_len);
    hdr->nb_segs = (uint8_t)(pkt->nb_segs + 1);

    /* copy metadata from source packet*/
    hdr->port = pkt->port;
    hdr->vlan_tci = pkt->vlan_tci;
    hdr->vlan_tci_outer = pkt->vlan_tci_outer;
    hdr->tx_offload = pkt->tx_offload;
    hdr->hash = pkt->hash;

    hdr->ol_flags = pkt->ol_flags;

    __rte_mbuf_sanity_check(hdr, 1);
    return (hdr);
}

// ------------------------------------------------------

static void dpdk_send_packet(struct rte_mbuf *mbuf, uint8_t port, uint32_t lcore_id)
{
    struct lcore_conf *conf = &lcore_conf[lcore_id];
    uint16_t queue_length = add_packet_to_queue(mbuf, port, lcore_id);

    if (unlikely(queue_length == MAX_PKT_BURST)) {
        debug("    :: BURST SENDING DPDK PACKETS - port:%d\n", port);
        send_burst(conf, MAX_PKT_BURST, port);
        queue_length = 0;
    }

    conf->hw.tx_mbufs[port].len = queue_length;
}

/* Enqueue a single packet, and send burst if queue is filled */
void send_single_packet(struct lcore_data* lcdata, packet_descriptor_t* pd, packet* pkt, int egress_port, int ingress_port, bool send_clone)
{
    uint32_t lcore_id = rte_lcore_id();
    struct rte_mbuf* mbuf = (struct rte_mbuf *)pkt;

    dpdk_send_packet(mbuf, egress_port, lcore_id);
}

// ------------------------------------------------------

void init_queues(struct lcore_data* lcdata) {
    for (unsigned i = 0; i < lcdata->conf->hw.n_rx_queue; i++) {
        unsigned portid = lcdata->conf->hw.rx_queue_list[i].port_id;
        uint8_t queueid = lcdata->conf->hw.rx_queue_list[i].queue_id;
        RTE_LOG(INFO, P4_FWD, " -- lcoreid=%u portid=%u rxqueueid=%hhu\n", rte_lcore_id(), portid, queueid);
    }
}

struct lcore_data init_lcore_data() {
    struct lcore_data lcdata = {
        .drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S * BURST_TX_DRAIN_US,
        .prev_tsc  = 0,

        .conf     = &lcore_conf[rte_lcore_id()],
        .mempool  = pktmbuf_pool[get_socketid(rte_lcore_id())], // TODO: Check for MULTI-SOCKET CASE !!!!

        .is_valid  = lcdata.conf->hw.n_rx_queue != 0,
    };

    if (lcdata.is_valid) {
        RTE_LOG(INFO, P4_FWD, "entering main loop on lcore %u\n", rte_lcore_id());

        init_queues(&lcdata);
    } else {
        RTE_LOG(INFO, P4_FWD, "lcore %u has nothing to do\n", rte_lcore_id());
    }

    return lcdata;
}

// ------------------------------------------------------

bool core_is_working(struct lcore_data* lcdata) {
    return true;
}

bool is_packet_handled(packet_descriptor_t* pd, struct lcore_data* lcdata) {
    return true;
}

bool receive_packet(packet_descriptor_t* pd, struct lcore_data* lcdata, unsigned pkt_idx) {
    packet* p = lcdata->pkts_burst[pkt_idx];
    rte_prefetch0(rte_pktmbuf_mtod(p, void *));
    pd->data = rte_pktmbuf_mtod(p, uint8_t *);
    pd->wrapper = p;

    return true;
}

void free_packet(packet_descriptor_t* pd) {
    rte_pktmbuf_free(pd->wrapper);
}


void init_storage() {
    /* Needed for L2 multicasting - e.g. acting as a hub
        cloning headers and sometimes packet data*/
    header_pool = rte_pktmbuf_pool_create("header_pool", NB_HDR_MBUF, 32,
            0, HDR_MBUF_DATA_SIZE, rte_socket_id());

    if (header_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot init header mbuf pool\n");

    clone_pool = rte_pktmbuf_pool_create("clone_pool", NB_CLONE_MBUF, 32,
            0, 0, rte_socket_id());

    if (clone_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot init clone mbuf pool\n");
}

void main_loop_pre_rx(struct lcore_data* lcdata) {
    tx_burst_queue_drain(lcdata);
}

void main_loop_post_rx(struct lcore_data* lcdata) {
}

void main_loop_post_single_rx(struct lcore_data* lcdata, bool got_packet) {
}

uint32_t get_portid(struct lcore_data* lcdata, unsigned queue_idx) {
    return lcdata->conf->hw.rx_queue_list[queue_idx].port_id;
}

void main_loop_rx_group(struct lcore_data* lcdata, unsigned queue_idx) {
    uint8_t queue_id = lcdata->conf->hw.rx_queue_list[queue_idx].queue_id;
    lcdata->nb_rx = rte_eth_rx_burst((uint8_t) get_portid(lcdata, queue_idx), queue_id, lcdata->pkts_burst, MAX_PKT_BURST);
}

unsigned get_pkt_count_in_group(struct lcore_data* lcdata) {
    return lcdata->nb_rx;
}

unsigned get_queue_count(struct lcore_data* lcdata) {
    return lcdata->conf->hw.n_rx_queue;
}

void initialize_nic() {
    dpdk_init_nic();
}

int launch_count() {
    return 1;
}

void t4p4s_abnormal_exit(int retval, int idx) {
    debug(T4LIT(Abnormal exit,error) ", code " T4LIT(%d) ".\n", retval);
}

void t4p4s_after_launch(int idx) {
    debug(T4LIT(Execution done.,success) "\n");
}

int t4p4s_normal_exit() {
    debug(T4LIT(Normal exit.,success) "\n");
    return 0;
}

void t4p4s_pre_launch(int idx) {
    
}

void t4p4s_post_launch(int idx) {

}

extern uint32_t enabled_port_mask;
uint32_t get_port_mask() {
    return enabled_port_mask;
}

extern uint8_t get_nb_ports();
uint8_t get_port_count() {
    return get_nb_ports();
}
