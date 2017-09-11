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
#include <unistd.h>

struct rte_mempool * pktmbuf_pool[NB_SOCKETS];
struct rte_mempool *header_pool, *clone_pool;
struct lcore_conf lcore_conf[RTE_MAX_LCORE];

//=   shared   ================================================================

uint32_t enabled_port_mask = 0;

//=============================================================================

struct rte_mempool* fakemempool;
struct rte_mbuf* fake_packet;
// struct rte_mbuf* fake_packets[2][4];

static
void str2bytes(const char* str, size_t len, char* res)
{
    const char *pos = str;
    size_t count = 0;
    for(count = 0; count < len; count++) {
        sscanf(pos, "%2hhx", &res[count]);
        pos += 2;
    }
}

static struct rte_mbuf*
fake_incoming_ipv4_packet(const char* ipv4_dstaddr)
{
    struct rte_mbuf *p = rte_pktmbuf_alloc(fakemempool);
    char* payload = rte_pktmbuf_prepend(p, 4);
    str2bytes("ffffffff", 4, payload);
    char* ipv4 = rte_pktmbuf_prepend(p, 20);
    str2bytes("450000280f444000800691f091fea0ed", 16, ipv4);
    str2bytes(ipv4_dstaddr, 4, ipv4+16);
//    char* ethernet2 = rte_pktmbuf_prepend(p, 14);
//    str2bytes("000002000000", 6, ethernet2+0);
//    str2bytes("000001000000", 6, ethernet2+6);
//    str2bytes("0800", 2, ethernet2+12);
    char* ethernet = rte_pktmbuf_prepend(p, 14);
    str2bytes("000002000000", 6, ethernet+0);
    str2bytes("000001000000", 6, ethernet+6);
    str2bytes("0800", 2, ethernet+12);
    return p;
}

/*
static void
init_fake_packets_l3()
{
    fakemempool = rte_mempool_create("test_mbuf_pool", (unsigned)1023, MBUF_SIZE, MEMPOOL_CACHE_SIZE, sizeof(struct rte_pktmbuf_pool_private), rte_pktmbuf_pool_init, NULL, rte_pktmbuf_init, NULL, 0, 0);

    fake_packet = fake_incoming_ipv4_packet("0A000001");

    //fake_packets[0][0] = fake_incoming_ipv4_packet("41d0e4df");
    //fake_packets[0][1] = fake_incoming_ipv4_packet("41d0e400");
    //fake_packets[1][0] = fake_incoming_ipv4_packet("41d0e4df");
    //fake_packets[1][1] = fake_incoming_ipv4_packet("41d00000");

    // TEACH THE DATAPLANE // 0A000101
    fake_packets[0][0] = fake_incoming_ipv4_packet("0A000001");
    fake_packets[0][1] = fake_incoming_ipv4_packet("0A000001");
    fake_packets[1][0] = fake_incoming_ipv4_packet("0A000001");
    fake_packets[1][1] = fake_incoming_ipv4_packet("0A000001");

    // CHECK IF IT KNOWS THE ADDRESSES NOW
    fake_packets[0][2] = fake_incoming_ipv4_packet("0A000001");
    fake_packets[0][3] = fake_incoming_ipv4_packet("0A000001");
    fake_packets[1][2] = fake_incoming_ipv4_packet("0A000001");
    fake_packets[1][3] = fake_incoming_ipv4_packet("0A000001");
}
*/

static void
init_fake_packet()
{
    fakemempool = rte_mempool_create("test_mbuf_pool", (unsigned)1023, MBUF_SIZE, MEMPOOL_CACHE_SIZE, sizeof(struct rte_pktmbuf_pool_private), rte_pktmbuf_pool_init, NULL, rte_pktmbuf_init, NULL, 0, 0);
    fake_packet = fake_incoming_ipv4_packet("0A000001");
}

//=============================================================================

#define EXTRACT_EGRESSPORT(p)  GET_INT32_AUTO_PACKET(p, header_instance_standard_metadata, field_standard_metadata_t_egress_port) 
#define EXTRACT_INGRESSPORT(p) GET_INT32_AUTO_PACKET(p, header_instance_standard_metadata, field_standard_metadata_t_ingress_port)

static inline int
send_packet(packet_descriptor_t* packet_desc)
{
    int port = EXTRACT_EGRESSPORT(packet_desc);
	uint32_t lcore_id;
	lcore_id = rte_lcore_id();
    printf("  :::: PACKET HANDLED SUCCESSFULLY BY LCORE %d, TRANSMITTING ON PORT %d\n", lcore_id, port);
	return 0;
}

static void
init_metadata(packet_descriptor_t* packet_desc, uint32_t inport)
{
    int res32;
    MODIFY_INT32_INT32_BITS_PACKET(packet_desc, header_instance_standard_metadata, field_standard_metadata_t_ingress_port, inport);
}

extern uint32_t read_counter (int counterid, int index);

static void print_mac(uint8_t* v) { printf("%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX\n", v[0], v[1], v[2], v[3], v[4], v[5]); }
static void print_ip(uint8_t* v) { printf("%d.%d.%d.%d\n",v[0],v[1],v[2],v[3]); }
void print_info(packet_descriptor_t *pd) {
    printf("------------------------------------------------------------\n");
    printf("PACKET INFO:\n");
    printf("dst mac: ");
    uint8_t* eth = pd->data;
    print_mac(eth);
    printf("src mac: ");
    print_mac(eth+6);
    uint8_t* ip = eth+14;//+14;
    printf("src ip: ");
    print_ip(ip+12);
    printf("dst ip: ");
    print_ip(ip+16);
    printf("ttl: %" PRIu8 "\n", *((uint8_t*)ip+8));
    printf("------------------------------------------------------------\n");
    int i;
    for(i = 0; i < NB_COUNTERS; i++) {
        printf("counter %s at index 0 equals %" PRIu32 "\n", counter_config[i].name, read_counter(i, 0));
        printf("------------------------------------------------------------\n");
    }
}

void
dpdk_main_loop(void)
{
    unsigned lcore_id = rte_lcore_id();
    if(lcore_id != 0) return;
    struct lcore_conf *conf = &lcore_conf[lcore_id];
	RTE_LOG(INFO, L2FWD, "entering main loop on lcore %u\n", lcore_id);
    struct rte_mbuf* p = fake_packet;
    packet_descriptor_t packet_desc;
    packet_desc.data = rte_pktmbuf_mtod(p, uint8_t *);
    packet_desc.wrapper = p;
    init_dataplane(&packet_desc, conf->state.tables);
    init_metadata(&packet_desc, (lcore_id*2)); // fake input port
	RTE_LOG(INFO, L2FWD, "fake packet to be handled by lcore %u\n", lcore_id);
    print_info(&packet_desc);
    handle_packet(&packet_desc, conf->state.tables);
    print_info(&packet_desc);
    send_packet(&packet_desc);
}


static int
launch_one_lcore(__attribute__((unused)) void *dummy)
{
	dpdk_main_loop();
	return 0;
}

int launch_dpdk()
{
    printf("Creating a fake IPv4 packet for test...\n");
    init_fake_packet();
    printf("Faked packet created.\n");
    printf("Executing packet handlers on each core...\n");
	rte_eal_mp_remote_launch(launch_one_lcore, NULL, CALL_MASTER);
	unsigned lcore_id;
	RTE_LCORE_FOREACH_SLAVE(lcore_id)
		if (rte_eal_wait_lcore(lcore_id) < 0)
			return -1;
	return 0;
}
