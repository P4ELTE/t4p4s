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

#include <netinet/ether.h> 

#include "dpdk_lib.h"
#include "util.h"
#include "dpdk_nicoff.h"

extern struct lcore_conf lcore_conf[RTE_MAX_LCORE];
extern void sleep_millis(int millis);

extern fake_cmd_t fake_commands[][RTE_MAX_LCORE];

// ------------------------------------------------------
// Exports

uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;

struct rte_mempool* mempools[RTE_MAX_LCORE];

// ------------------------------------------------------
// Helpers

uint8_t bytes[8*sizeof(struct ether_header)];

// str is a string that contains hex numbers without spaces.
// Their values are copied as bytes into dst in a linear fashion,
// and the pointer after the last written position is retured.
static uint8_t* str2bytes(const char* str, uint8_t* dst)
{
    size_t len = strlen(str) / 2;

    const char *pos = str;
    for (size_t count = 0; count < len; count++) {
        sscanf(pos, "%2hhx", dst);
        pos += 2;
        ++dst;
    }

    return dst;
}


#define MAX_PACKET_SIZE 128
uint8_t tmp[MAX_PACKET_SIZE];

int total_fake_byte_count(char* texts[MAX_SECTION_COUNT]) {
    int retval = 0;
    while (strlen(*texts) > 0) {
        retval += strlen(*texts) / 2;
        ++texts;
    }

    return retval;
}


struct rte_mbuf* fake_packet(char* texts[MAX_SECTION_COUNT]) {
    int byte_count = total_fake_byte_count(texts);

    struct rte_mbuf* p  = rte_pktmbuf_alloc(mempools[rte_lcore_id()]);
    uint8_t*         p2 = (uint8_t*)rte_pktmbuf_prepend(p, byte_count);
    while (strlen(*texts) > 0) {
        p2 = str2bytes(*texts, p2);
        ++texts;
    }

    return p;
}

// ------------------------------------------------------
// TODO

fake_cmd_t get_cmd(struct lcore_data* lcdata) {
    return fake_commands[rte_lcore_id()][lcdata->iteration_idx];
}


bool core_is_working(struct lcore_data* lcdata) {
    return get_cmd(lcdata).action != FAKE_END;
}

bool receive_packet(packet_descriptor_t* pd, struct lcore_data* lcdata, unsigned pkt_idx) {
    fake_cmd_t cmd = get_cmd(lcdata);
    if (cmd.action == FAKE_PKT) {
        bool got_packet = strlen(cmd.in[0]) > 0;

        if (got_packet) {
            pd->wrapper = fake_packet(cmd.in);
            pd->data    = rte_pktmbuf_mtod(pd->wrapper, uint8_t*);
        }

        if (cmd.sleep_millis > 0) {
            sleep_millis(cmd.sleep_millis);
        }

        return got_packet;
    }

    return false;
}

void free_packet(packet_descriptor_t* pd) {
    rte_free(pd->wrapper);
}

bool is_packet_handled(packet_descriptor_t* pd, struct lcore_data* lcdata) {
    return get_cmd(lcdata).action == FAKE_PKT;
}

void main_loop_pre_rx(struct lcore_data* lcdata) {

}

void main_loop_post_rx(struct lcore_data* lcdata) {

}

void main_loop_post_single_rx(struct lcore_data* lcdata, bool got_packet) {
    ++lcdata->iteration_idx;
}

unsigned get_portid(struct lcore_data* lcdata, unsigned queue_idx) {
    // TODO
    return 1;
}

void main_loop_rx_group(struct lcore_data* lcdata, unsigned queue_idx) {

}

unsigned get_pkt_count_in_group(struct lcore_data* lcdata) {
    return 1;
}

unsigned get_queue_count(struct lcore_data* lcdata) {
    return 1;
}

void send_packet(packet_descriptor_t* pd, int egress_port, int ingress_port) {
    struct rte_mbuf* mbuf = (struct rte_mbuf *)pd->wrapper;
    dbg_bytes(rte_pktmbuf_mtod(mbuf, uint8_t*), rte_pktmbuf_pkt_len(mbuf), "Emitting packet on port %d (%d bytes): ", egress_port, rte_pktmbuf_pkt_len(mbuf));
}

void init_mempools() {
    char str[15];
    for (int i = 0; i < RTE_MAX_LCORE; ++i) {
        sprintf(str, "testpool_%d", i);
        mempools[i] = rte_mempool_create(str, (unsigned)1023, MBUF_SIZE, MEMPOOL_CACHE_SIZE, sizeof(struct rte_pktmbuf_pool_private), rte_pktmbuf_pool_init, NULL, rte_pktmbuf_init, NULL, 0, 0);
    }
}

void init_service() {
    init_mempools();
}

struct lcore_data init_lcore_data() {
    return (struct lcore_data) {
        .conf          = &lcore_conf[rte_lcore_id()],
        .is_valid      = true,
        .iteration_idx = 0,
    };
}
