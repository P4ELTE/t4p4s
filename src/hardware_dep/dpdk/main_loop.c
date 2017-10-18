/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2015 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
// A large portion of the code in this file comes from
// main.c in the l3fwd example of DPDK 2.2.0.

#include "dpdk_lib.h"

#include <rte_ethdev.h>

struct rte_mempool *header_pool, *clone_pool;
struct lcore_conf lcore_conf[RTE_MAX_LCORE];

extern void p4_handle_packet(packet* p, unsigned portid);

//=   shared   ================================================================

uint32_t enabled_port_mask = 0;

//=   used only here   ========================================================


extern unsigned int rx_queue_per_lcore;

/* A tsc-based timer responsible for triggering statistics printout */
#define TIMER_MILLISECOND 2000000ULL /* around 1ms at 2 Ghz */
#define MAX_TIMER_PERIOD 86400 /* 1 day max */
int64_t timer_period = 10 * TIMER_MILLISECOND * 1000; /* default period is 10 seconds */

#define MAX_PORTS 16

#define MCAST_CLONE_PORTS       2
#define MCAST_CLONE_SEGS        2

#define RTE_TEST_RX_DESC_DEFAULT 128
#define RTE_TEST_TX_DESC_DEFAULT 512
uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;


#define PKT_MBUF_DATA_SIZE      RTE_MBUF_DEFAULT_BUF_SIZE
#define NB_PKT_MBUF     8192

#define HDR_MBUF_DATA_SIZE      (2 * RTE_PKTMBUF_HEADROOM)
#define NB_HDR_MBUF     (NB_PKT_MBUF * MAX_PORTS)

#define NB_CLONE_MBUF   (NB_PKT_MBUF * MCAST_CLONE_PORTS * MCAST_CLONE_SEGS * 2)

#define BURST_TX_DRAIN_US 100 /* TX drain every ~100us */



// note: this much space MUST be able to hold all deparsed content
#define DEPARSE_BUFFER_SIZE 1024
struct rte_mbuf* deparse_mbuf;

static const struct rte_eth_conf port_conf = {
    .rxmode = {
        .split_hdr_size = 0,
        .header_split   = 0, /**< Header Split disabled */
        .hw_ip_checksum = OFFLOAD_CHECKSUM, /**< IP checksum offload disabled */
        .hw_vlan_filter = 0, /**< VLAN filtering disabled */
        .jumbo_frame    = 0, /**< Jumbo Frame Support disabled */
        .hw_strip_crc   = 0, /**< CRC stripped by hardware */
    },
    .txmode = {
        .mq_mode = ETH_MQ_TX_NONE,
    },
};

struct rte_mempool * pktmbuf_pool[NB_SOCKETS];

//=============================================================================




/* Send burst of packets on an output interface */
static inline int
send_burst(struct lcore_conf *qconf, uint16_t n, uint8_t port)
{
    struct rte_mbuf **m_table;
    int ret;
    uint16_t queueid;

    queueid = qconf->tx_queue_id[port];
    m_table = (struct rte_mbuf **)qconf->tx_mbufs[port].m_table;

    ret = rte_eth_tx_burst(port, queueid, m_table, n);
    if (unlikely(ret < n)) {
        do {
            rte_pktmbuf_free(m_table[ret]);
        } while (++ret < n);
    }

    return 0;
}

/* Send burst of outgoing packet, if timeout expires. */
static inline void
send_timeout_burst(struct lcore_conf *qconf)
{
        uint64_t cur_tsc;
        uint8_t portid;
        const uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S * BURST_TX_DRAIN_US;

        cur_tsc = rte_rdtsc();
        if (likely (cur_tsc < qconf->tx_tsc + drain_tsc))
            return;

        for (portid = 0; portid < MAX_PORTS; portid++) {
            if (qconf->tx_mbufs[portid].len != 0) {
                send_burst(qconf, qconf->tx_mbufs[portid].len, portid);
                qconf->tx_mbufs[portid].len = 0; 
            }
        }
        qconf->tx_tsc = cur_tsc;
}



static int
get_socketid(unsigned lcore_id)
{
    if (numa_on)
        return rte_lcore_to_socket_id(lcore_id);
    else
        return 0;
}


static inline void
dbg_print_headers(packet_descriptor_t* pd)
{
    for (int i = 0; i < HEADER_INSTANCE_COUNT; ++i) {
        debug("    :: header %d (type=%d, len=%d) = ", i, pd->headers[i].type, pd->headers[i].length);
        for (int j = 0; j < pd->headers[i].length; ++j) {
            debug("%02x ", ((uint8_t*)(pd->headers[i].pointer))[j]);
        }
        debug("\n");
    }
}

static inline unsigned
deparse_headers(packet_descriptor_t* pd, int socketid)
{
    uint8_t* deparse_buffer = (uint8_t*)rte_pktmbuf_append(deparse_mbuf, DEPARSE_BUFFER_SIZE);
    int len = 0;
    for (int i = 0; i < HEADER_INSTANCE_COUNT; ++i) {
        uint8_t* hdr_ptr = (uint8_t*)(pd->headers[i].pointer);
        unsigned hdr_len = pd->headers[i].length;
        for (int j = 0; j < hdr_len; ++j) {
            *deparse_buffer = *hdr_ptr;
            ++deparse_buffer;
            ++hdr_ptr;
        }
        len += hdr_len;
    }
    return len;
}

/* Get number of bits set. */
static inline uint32_t
bitcnt(uint32_t v)
{
        uint32_t n;

        for (n = 0; v != 0; v &= v - 1, n++)
                ;

        return (n);
}

static void
dpdk_send_packet(struct rte_mbuf *m, uint8_t port, uint32_t lcore_id)
{
    struct lcore_conf *qconf = &lcore_conf[lcore_id];
    uint16_t len = qconf->tx_mbufs[port].len;
    qconf->tx_mbufs[port].m_table[len] = m;
    len++;

    if (unlikely(len == MAX_PKT_BURST)) {
        debug("    :: BURST SENDING DPDK PACKETS - port:%d\n", port);
        send_burst(qconf, MAX_PKT_BURST, port);
        len = 0;
    }

    qconf->tx_mbufs[port].len = len;
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

static void
dpdk_mcast_packet(struct rte_mbuf *m, uint32_t port_mask, uint32_t lcore_id)
{
    struct rte_mbuf *mc;
    uint32_t port_num, use_clone;
    uint8_t port;

    port_num = bitcnt(port_mask);

    /* Should we use rte_pktmbuf_clone() or not. */
    use_clone = (port_num <= MCAST_CLONE_PORTS &&
        m->nb_segs <= MCAST_CLONE_SEGS);

    /* Mark all packet's segments as referenced port_num times */
    if (use_clone == 0)
            rte_pktmbuf_refcnt_update(m, (uint16_t)port_num);

    debug("USE_CLONE = %d\n", use_clone);

    for (port = 0; use_clone != port_mask; port_mask >>= 1, port++) {
        /* Prepare output packet and send it out. */
        if ((port_mask & 1) != 0) {
            debug("MCAST - PORT -%d\n", port);
            if ((mc = mcast_out_pkt(m, use_clone)) != NULL) {
                debug("MCAST mc is ready\n");
                dpdk_send_packet(mc, port, lcore_id);
            } else if (use_clone == 0) {
                rte_pktmbuf_free(m);
            }
        }
    }

    /*
     * If we making clone packets, then, for the last destination port,
     * we can overwrite input packet's metadata.
     */
    if (use_clone != 0)
        dpdk_send_packet(m, port, lcore_id);
    else
        rte_pktmbuf_free(m);
}

static void
dpdk_bcast_packet(struct rte_mbuf *m, uint8_t ingress_port, uint32_t lcore_id)
{
    struct rte_mbuf *mc;
    uint32_t port_num;
    uint8_t port; //,portid;
    port_num = 2; // TODO: update

    debug("Broadcast - ingress port:%d/%d\n", ingress_port, port_num);

    /* Mark all packet's segments as referenced port_num times */
//        rte_pktmbuf_refcnt_update(m, (uint16_t)port_num);

    for (port = 0; port<port_num; port++) {
        /* Prepare output packet and send it out. */
        if (port != ingress_port) {
                if ((mc = mcast_out_pkt(m, 1)) != NULL)
                        dpdk_send_packet(mc, port, lcore_id);
        }
    }

    /*
     * If we making clone packets, then, for the last destination port,
     * we can overwrite input packet's metadata.
     */
     rte_pktmbuf_free(m);
}

#define EXTRACT_EGRESSPORT(p)  GET_INT32_AUTO_PACKET(p, header_instance_standard_metadata, field_standard_metadata_t_egress_port) 
#define EXTRACT_INGRESSPORT(p) GET_INT32_AUTO_PACKET(p, header_instance_standard_metadata, field_standard_metadata_t_ingress_port)


/* Enqueue a single packet, and send burst if queue is filled */
static inline int
send_packet(packet_descriptor_t* pd)
{
    if (pd->dropped) {
        debug("  :::: DROPPING\n");
    } else {
        int port = EXTRACT_EGRESSPORT(pd);
        int inport = EXTRACT_INGRESSPORT(pd);

        uint32_t lcore_id = rte_lcore_id();

        debug("  :::: EGRESSING\n");
        debug("    :: deparsing headers\n");
        debug("    :: sending packet on port %d (lcore %d)\n", port, lcore_id);

        if (port==100)
            dpdk_bcast_packet((struct rte_mbuf *)pd->wrapper, inport, lcore_id);
        else
            dpdk_send_packet((struct rte_mbuf *)pd->wrapper, port, lcore_id);
    }
    return 0;
}

static void
set_metadata_inport(packet_descriptor_t* packet_desc, uint32_t inport)
{
    //modify_field_to_const(packet_desc, field_desc(field_instance_standard_metadata_ingress_port), (uint8_t*)&inport, 2);
    int res32; // needed for the macro
    MODIFY_INT32_INT32_BITS_PACKET(packet_desc, header_instance_standard_metadata, field_standard_metadata_t_ingress_port, inport);
    //MODIFY_INT32_INT32_BITS(packet_desc, field_instance_standard_metadata_ingress_port, inport); // TODO fix? LAKI
}

void
packet_received(packet_descriptor_t* pd, packet *p, unsigned portid, struct lcore_conf *conf)
{
    pd->data = rte_pktmbuf_mtod(p, uint8_t *);
    pd->wrapper = p;
    set_metadata_inport(pd, portid);
    handle_packet(pd, conf->state.tables);
    send_packet(pd);
}

void
dpdk_main_loop(void)
{
    packet *pkts_burst[MAX_PKT_BURST];
    packet *p;
    uint64_t prev_tsc, diff_tsc, cur_tsc;
    unsigned i, j, portid, nb_rx;
    const uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S * BURST_TX_DRAIN_US;
    uint8_t queueid;

    prev_tsc = 0;

    unsigned lcore_id = rte_lcore_id();
    struct lcore_conf *qconf = &lcore_conf[lcore_id];

    if (qconf->n_rx_queue == 0) {
        RTE_LOG(INFO, P4_FWD, "lcore %u has nothing to do\n", lcore_id);
        return;
    }

    RTE_LOG(INFO, P4_FWD, "entering main loop on lcore %u\n", lcore_id);

    for (i = 0; i < qconf->n_rx_queue; i++) {

        portid = qconf->rx_queue_list[i].port_id;
        queueid = qconf->rx_queue_list[i].queue_id;
        RTE_LOG(INFO, P4_FWD, " -- lcoreid=%u portid=%u rxqueueid=%hhu\n", lcore_id, portid, queueid);
    }

    /*struct lcore_conf *conf = &lcore_conf[lcore_id];*/

    packet_descriptor_t pd;
    init_dataplane(&pd, qconf->state.tables);

    while (1) {

        cur_tsc = rte_rdtsc();

        /*
         * TX burst queue drain
         */
        diff_tsc = cur_tsc - prev_tsc;
        if (unlikely(diff_tsc > drain_tsc)) {

            for (portid = 0; portid < RTE_MAX_ETHPORTS; portid++) {
                if (qconf->tx_mbufs[portid].len == 0)
                    continue;
                send_burst(qconf,
                         qconf->tx_mbufs[portid].len,
                         (uint8_t) portid);
                qconf->tx_mbufs[portid].len = 0;
            }

            prev_tsc = cur_tsc;
        }

        /*
         * Read packet from RX queues
         */
        for (i = 0; i < qconf->n_rx_queue; i++) {

            portid = qconf->rx_queue_list[i].port_id;
            queueid = qconf->rx_queue_list[i].queue_id;
            nb_rx = rte_eth_rx_burst((uint8_t) portid, queueid,
                         pkts_burst, MAX_PKT_BURST);

            for (j = 0; j < nb_rx; j++) {
                p = pkts_burst[j];
                rte_prefetch0(rte_pktmbuf_mtod(p, void *));

                packet_received(&pd, p, portid, qconf);
            }
        }
    }
}


static int
launch_one_lcore(__attribute__((unused)) void *dummy)
{
    dpdk_main_loop();
    return 0;
}

int launch_dpdk()
{

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

    rte_eal_mp_remote_launch(launch_one_lcore, NULL, CALL_MASTER);

    unsigned lcore_id;
    RTE_LCORE_FOREACH_SLAVE(lcore_id) {
        if (rte_eal_wait_lcore(lcore_id) < 0)
            return -1;
    }
    return 0;
}
