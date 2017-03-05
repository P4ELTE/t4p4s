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
fake_incoming_ipv4_packet()
{
    struct rte_mbuf *p = rte_pktmbuf_alloc(fakemempool);

    #define TEST_PACKET_NUMBER 1
    
    #if TEST_PACKET_NUMBER == 1
        char* payload = rte_pktmbuf_prepend(p, 54);
        str2bytes("1703030031ec1d1a9a53b460e99ca9f397165069c993db9c7ac6961a7d821c0c2486c13cd501afde1beda6fc4969dacadd1e318e6b8b", 54, payload);

        char* tcp = rte_pktmbuf_prepend(p, 32);
        str2bytes("01bbcf0ea07c2320528c2d6b8018002eaf0d00000101080a989199cb00280afd", 32, tcp);

        char* ipv4 = rte_pktmbuf_prepend(p, 24);
        str2bytes("4600006a777540003106aa1dc284a263c0a8016b00000000", 24, ipv4);
    #elif TEST_PACKET_NUMBER == 2
        //Test case where ipv4.options is not zero
        char* full = rte_pktmbuf_prepend(p, 32);
        str2bytes("46c000200000400001024109c0a8016be00000fb9404000016000904e00000fb", 32, full);
    #elif TEST_PACKET_NUMBER == 3
        char* full = rte_pktmbuf_prepend(p, 40);
        str2bytes("45000028e90b40004006f90cc0a8016bc629d07aec6401bb58526ec04df6142d501000e53ee20000", 40, full);
    #else
        char* full = rte_pktmbuf_prepend(p, 83);
        str2bytes("45000053617240003906852e9765018cc0a8016b01bb9140c7429b41eca070fd8018003d1e1e00000101080aa33aebd300468825150303001a9eeb2d5965840121f6654196bed15b97011d374df64b5d6689f4", 83, full);
    #endif
    char* ethernet = rte_pktmbuf_prepend(p, 14);
    str2bytes("000002000000", 6, ethernet+0);
    str2bytes("000001000000", 6, ethernet+6);
    str2bytes("0800", 2, ethernet+12);

    return p;
}

static void
init_fake_packet()
{
    fakemempool = rte_mempool_create("test_mbuf_pool", (unsigned)1023, MBUF_SIZE, MEMPOOL_CACHE_SIZE, sizeof(struct rte_pktmbuf_pool_private), rte_pktmbuf_pool_init, NULL, rte_pktmbuf_init, NULL, 0, 0);
    fake_packet = fake_incoming_ipv4_packet();
}

//=============================================================================

#define EXTRACT_EGRESSPORT(p) (*(uint32_t *)(((uint8_t*)(p)->headers[/*header instance id - hopefully it's the very first one*/0].pointer)+/*byteoffset*/6) & /*mask*/0x7fc) >> /*bitoffset*/2

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
    MODIFY_INT32_INT32_BITS(packet_desc, field_instance_standard_metadata_ingress_port, inport);
}

extern uint32_t read_counter (int counterid, int index);

static void print_mac(uint8_t* v) { printf("%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX\n", v[0], v[1], v[2], v[3], v[4], v[5]); }
static void print_ip(uint8_t* v) { printf("%d.%d.%d.%d\n",v[0],v[1],v[2],v[3]); }

static void print_bytes_hex(uint8_t* v, size_t c) {
    int i;
    printf("0x");
    for(i = 0; i < c; ++i)
        printf("%02x ", v[i]);
    printf("\n");
}

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

    //Print the options field of the ipv4 packet
    uint8_t ihl = ip[0] & 0x0f;
    printf("options: ");
    print_bytes_hex(ip+20, (4*ihl)-20);

    printf("------------------------------------------------------------\n");
    int i;
    for(i = 0; i < NB_COUNTERS; i++) {
        printf("counter %s at index 0 equals %" PRIu32 "\n", counter_config[i].name, read_counter(i, 0));
        printf("------------------------------------------------------------\n");
    }
}

extern void ipv4_fib_lpm_set_default_table_action(struct p4_ctrl_msg* ctrl_m);

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

    //Set the default action for table ipv4_fib_lpm to on_miss
    struct p4_ctrl_msg msg;
    char* action_name = "on_miss";
    msg.action_name = action_name;
    ipv4_fib_lpm_set_default_table_action(&msg);

    print_info(&packet_desc);
    handle_packet(&packet_desc, conf->state.tables);
    print_info(&packet_desc);
    uint32_t tmp = 0;
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
