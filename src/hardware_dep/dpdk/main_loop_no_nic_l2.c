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
#include <string.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <netinet/ether.h> 
#include "util.h"

#include <stdio.h> 

struct rte_mempool * pktmbuf_pool[NB_SOCKETS];
struct lcore_conf lcore_conf[RTE_MAX_LCORE];
uint32_t enabled_port_mask = 0;

// =============================================================================
// Fake incoming bytes

uint8_t bytes[8*sizeof(struct ether_header)];

static void
str2bytes(const char* str, size_t len, uint8_t* res)
{
    const char *pos = str;
    size_t count = 0;
    for(count = 0; count < len; count++) {
        sscanf(pos, "%2hhx", &res[count]);
        pos += 2;
    }
}

int offset = 0;

static void
fake_eth_packet(const char* dst, const char* src)
{
    struct ether_header *eh = (struct ether_header *) (bytes+offset);
    str2bytes(dst, 6, (uint8_t*)eh->ether_dhost);
    str2bytes(src, 6, (uint8_t*)eh->ether_shost);
    eh->ether_type = htons(ETH_P_IP);
    offset += sizeof(struct ether_header);
}

struct fake_output_data {
    int len;
    uint8_t* data;
};

uint8_t fake_expected_output01[] = {0x78, 0x56, 0x34, 0x12, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00};
uint8_t fake_expected_output02[] = {0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00};
uint8_t fake_expected_output03[] = {0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00};
uint8_t fake_expected_output04[] = {0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00};

uint8_t fake_expected_output11[] = {0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00};
uint8_t fake_expected_output12[] = {0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00};
uint8_t fake_expected_output13[] = {0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00};
uint8_t fake_expected_output14[] = {0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0x90, 0x08, 0x00};

struct fake_output_data fake_expected_output0[] = {
    {42, fake_expected_output01},
    {42, fake_expected_output02},
    {42, fake_expected_output03},
    {42, fake_expected_output04},
};

struct fake_output_data fake_expected_output1[] = {
    {42, fake_expected_output11},
    {42, fake_expected_output12},
    {42, fake_expected_output13},
    {42, fake_expected_output14},
};

static void
init_fake_packets_l2()
{
    // for lcore 0
    fake_eth_packet("001234567890", "000001000000");
    fake_eth_packet("001234567890", "000002000000");
    fake_eth_packet("000001000000", "001234567890");
    fake_eth_packet("000002000000", "001234567890");
    // for lcore 1
    fake_eth_packet("001234567890", "000003000000");
    fake_eth_packet("001234567890", "000004000000");
    fake_eth_packet("000003000000", "001234567890");
    fake_eth_packet("000004000000", "001234567890");
}

struct rte_mempool* mempools[RTE_MAX_LCORE];

// =============================================================================
// Fake DPDK packets

static void init_mem()
{
    int i;
    char str[15];
    for(i = 0; i < RTE_MAX_LCORE; i++) {
        sprintf(str, "testpool_%d", i);
        mempools[i] = rte_mempool_create(str, (unsigned)1023, MBUF_SIZE, MEMPOOL_CACHE_SIZE, sizeof(struct rte_pktmbuf_pool_private), rte_pktmbuf_pool_init, NULL, rte_pktmbuf_init, NULL, 0, 0);
    }
}

static struct rte_mbuf*
fake_dpdk_packet(unsigned lcore_id, int index)
{
    int offset = ((lcore_id * 4) + index)*sizeof(struct ether_header);
    struct rte_mbuf *p = rte_pktmbuf_alloc(mempools[lcore_id]);
    char* p2 = rte_pktmbuf_prepend(p, sizeof(struct ether_header)); 
    memcpy(p2, bytes+offset, sizeof(struct ether_header));
    return p;
}

// =============================================================================

#define EXTRACT_EGRESSPORT(p)  GET_INT32_AUTO_PACKET(p, header_instance_standard_metadata, field_standard_metadata_t_egress_port)

static void
send_packet(struct rte_mbuf * mbuf, int egress_port)
{
    dbg_bytes(rte_pktmbuf_mtod(mbuf, uint8_t*), rte_pktmbuf_pkt_len(mbuf), "Emitting packet on port %d (%d bytes): ", egress_port, rte_pktmbuf_pkt_len(mbuf));

    rte_pktmbuf_free(mbuf);
}

static void
init_metadata(packet_descriptor_t* packet_desc, uint32_t inport)
{
    int res32;
    MODIFY_INT32_INT32_BITS_PACKET(packet_desc, header_instance_standard_metadata, field_standard_metadata_t_ingress_port, inport);
}

void
dpdk_main_loop(void)
{
    unsigned lcore_id = rte_lcore_id();
    struct lcore_conf *conf = &lcore_conf[lcore_id];
#ifdef P4DPDK_DEBUG
	debug("entering main loop on lcore %u\n", lcore_id);
#endif
    int index = 0;

    packet_descriptor_t pd;
    init_dataplane(&pd, conf->state.tables);

	while (index < 4) { 
        pd.wrapper = fake_dpdk_packet(lcore_id, index);
        pd.data = rte_pktmbuf_mtod(pd.wrapper, uint8_t*);

        init_metadata(&pd, (lcore_id*2+index+1)); // fake input port

        handle_packet(&pd, conf->state.tables);

        int egress_port = EXTRACT_EGRESSPORT(&pd);
        send_packet(pd.wrapper, egress_port);

        if(index == 1) {
            debug("PACKET_GEN IS WAITING FOR THE DATAPLANE TO LEARN\n");
            sleep(1);
        }
        index++;
    }

    debug("Core is done\n");
}

static int
launch_one_lcore(__attribute__((unused)) void *dummy)
{
	dpdk_main_loop();
	return 0;
}

int launch_dpdk()
{
    debug("Creating fake packets for test...\n");
    init_fake_packets_l2();
    debug("Faked packets created.\n");
    init_mem();
    debug("Executing packet handlers on each core...\n");
	rte_eal_mp_remote_launch(launch_one_lcore, NULL, CALL_MASTER);
	unsigned lcore_id;
	RTE_LCORE_FOREACH_SLAVE(lcore_id)
		if (rte_eal_wait_lcore(lcore_id) < 0)
			return -1;
	return 0;
}

