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

extern int get_socketid(unsigned lcore_id);

extern struct lcore_conf lcore_conf[RTE_MAX_LCORE];
extern void sleep_millis(int millis);
extern void dpdk_init_nic();

extern struct rte_mempool* pktmbuf_pool[NB_SOCKETS];

// ------------------------------------------------------
// Exports

uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;

// ------------------------------------------------------
// Test cases

testcase_t* current_test_case;

#ifdef T4P4S_TEST_SUITE
    extern testcase_t t4p4s_test_suite[MAX_TESTCASES];

    void t4p4s_pre_launch(int idx) {
        if (idx != 0) {
            sleep_millis(PAUSE_BETWEEN_TESTCASES_MILLIS);
        }

        current_test_case = t4p4s_test_suite + idx;

        debug("------------------------------------------------\n");
        debug("Executing test case " T4LIT(%s,testcase) "\n", current_test_case->name);
    }

    // A testcase_t with a null pointer as `steps` terminates the suite.
    int launch_count() {
        int idx = 0;
        testcase_t* suite = t4p4s_test_suite;
        while (suite[idx].steps)   ++idx;
        return idx;
    }
#else
    #ifndef T4P4S_TESTCASE
    #error "One of these two macros must be defined: T4P4S_TEST_SUITE, T4P4S_TESTCASE"
    #endif

    #define TOSTRING(x) #x

    extern fake_cmd_t T4P4S_TESTCASE[][RTE_MAX_LCORE];

    testcase_t single_test_case = {
        .name  = TOSTRING(T4P4S_TESTCASE),
        .steps = &T4P4S_TESTCASE,
    };

    void t4p4s_pre_launch(int idx) {
        current_test_case = &single_test_case;
    }

    int launch_count() {
        return 1;
    }
#endif

// ------------------------------------------------------
// Helpers

fake_cmd_t get_cmd(struct lcore_data* lcdata) {
    return (*(current_test_case->steps))[rte_lcore_id()][lcdata->idx];
}


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


#define MAX_PACKET_SIZE 1024
uint8_t tmp[MAX_PACKET_SIZE];

int total_fake_byte_count(const char* texts[MAX_SECTION_COUNT]) {
    int retval = 0;
    while (strlen(*texts) > 0) {
        retval += strlen(*texts) / 2;
        ++texts;
    }

    return retval;
}


struct rte_mbuf* fake_packet(struct lcore_data* lcdata, const char* texts[MAX_SECTION_COUNT]) {
    int byte_count = total_fake_byte_count(texts);

    debug("Creating fake " T4LIT(packet #%d,packet) " (" T4LIT(%d) " bytes)\n", lcdata->pkt_idx + 1, byte_count);

    struct rte_mbuf* p  = rte_pktmbuf_alloc(pktmbuf_pool[get_socketid(rte_lcore_id())]);
    uint8_t*         p2 = (uint8_t*)rte_pktmbuf_prepend(p, byte_count);
    while (strlen(*texts) > 0) {
        uint8_t* dst = p2;
        p2 = str2bytes(*texts, p2);

        dbg_bytes(dst, strlen(*texts) / 2, " :::: " T4LIT(%2zd) " bytes: ", strlen(*texts) / 2);

        ++texts;
    }

    return p;
}

void abort_on_strict() {
#ifdef T4P4S_STRICT
    rte_exit(1, "Unexpected data, " T4LIT(aborting,error) "\n");
#endif
}

void check_egress_port(struct lcore_data* lcdata, fake_cmd_t cmd, int egress_port) {
    if (cmd.out_port != egress_port) {
        debug(" " T4LIT(!!!!,error) " " T4LIT(packet #%d,packet) "@" T4LIT(core%d,core) ": expected egress port is " T4LIT(%d,expected) ", got " T4LIT(%d,error) "\n",
              lcdata->pkt_idx + 1, rte_lcore_id(),
              cmd.out_port, egress_port);
        lcdata->is_valid = false;
        abort_on_strict();
    }
}

bool check_byte_count(struct lcore_data* lcdata, fake_cmd_t cmd, packet_descriptor_t* pd) {
    struct rte_mbuf* mbuf = (struct rte_mbuf*)pd->wrapper;

    int expected_byte_count = total_fake_byte_count(cmd.out);
    int actual_byte_count = rte_pktmbuf_pkt_len(mbuf);
    if (expected_byte_count != actual_byte_count) {
        debug(" " T4LIT(!!!!,error) " " T4LIT(packet #%d,packet) "@" T4LIT(core%d,core) ": expected " T4LIT(%d,expected) " bytes, got " T4LIT(%d,error) "\n",
              lcdata->pkt_idx + 1, rte_lcore_id(),
              expected_byte_count, actual_byte_count);

        lcdata->is_valid = false;
        abort_on_strict();
        return false;
    }
    return true;
}

int get_wrong_byte_count(struct lcore_data* lcdata, fake_cmd_t cmd, packet_descriptor_t* pd) {
    int wrong_byte_count = 0;

    int byte_idx = 0;
    int section_idx = 0;
    const char** texts = cmd.out;
    while (strlen(*texts) > 0) {
        for (size_t i = 0; i < strlen(*texts) / 2; ++i) {
            uint8_t expected_byte;
            sscanf(*texts + (2*i), "%2hhx", &expected_byte);

            uint8_t actual_byte = pd->data[byte_idx];
            if (expected_byte != actual_byte) {
                ++wrong_byte_count;
            }

            ++byte_idx;
        }

        ++texts;
        ++section_idx;
    }

    return wrong_byte_count;
}

#define MSG_MAX_LEN 4096

void print_wrong_bytes(struct lcore_data* lcdata, fake_cmd_t cmd, packet_descriptor_t* pd, int wrong_byte_count) {
    char msg[MSG_MAX_LEN];
    char* msgptr = msg;

    int byte_idx = 0;
    int section_idx = 0;
    const char** texts = cmd.out;
    while (strlen(*texts) > 0) {
        sprintf(msgptr, "[");
        msgptr += 1;

        for (size_t i = 0; i < strlen(*texts) / 2; ++i) {
            uint8_t expected_byte;
            sscanf(*texts + (2*i), "%2hhx", &expected_byte);

            uint8_t actual_byte = pd->data[byte_idx];
            int written_bytes;
            if (expected_byte != actual_byte) {
                written_bytes = sprintf(msgptr, T4LIT(%02x,error), actual_byte);
            } else {
                written_bytes = sprintf(msgptr, T4LIT(%02x,expected), actual_byte);
            }
            msgptr += written_bytes;

            ++byte_idx;
        }

        sprintf(msgptr, "]");
        msgptr += 1;

        ++texts;
        ++section_idx;
    }

    msgptr[0] = 0;
    debug(" " T4LIT(!!!!,error) " " T4LIT(%d) " wrong bytes found: %s\n", wrong_byte_count, msg);
}

void check_packet_contents(struct lcore_data* lcdata, fake_cmd_t cmd, packet_descriptor_t* pd) {
    int wrong_byte_count = get_wrong_byte_count(lcdata, cmd, pd);

    if (wrong_byte_count != 0) {
        print_wrong_bytes(lcdata, cmd, pd, wrong_byte_count);
        lcdata->is_valid = false;
        abort_on_strict();
    }
}

void check_sent_packet(struct lcore_data* lcdata, packet_descriptor_t* pd, int egress_port, int ingress_port) {
    fake_cmd_t cmd = get_cmd(lcdata);

    check_egress_port(lcdata, cmd, egress_port);
    bool is_ok = check_byte_count(lcdata, cmd, pd);
    if (is_ok) {
        check_packet_contents(lcdata, cmd, pd);
    }

#ifdef T4P4S_DEBUG
    if (lcdata->is_valid) {
        debug( " :::: " T4LIT(Packet #%d,packet) "@" T4LIT(core%d,core) " is " T4LIT(sent successfully,success) "\n", lcdata->pkt_idx + 1, rte_lcore_id());
    } else {
        debug( " " T4LIT(!!!!,error)" " T4LIT(Packet #%d,packet) "@" T4LIT(core%d,core) " is " T4LIT(sent with errors,error) "\n", lcdata->pkt_idx + 1, rte_lcore_id());
    }
#endif
}

// ------------------------------------------------------
// TODO

bool core_is_working(struct lcore_data* lcdata) {
    return get_cmd(lcdata).action != FAKE_END;
}

bool receive_packet(packet_descriptor_t* pd, struct lcore_data* lcdata, unsigned pkt_idx) {
    fake_cmd_t cmd = get_cmd(lcdata);
    if (cmd.action == FAKE_PKT) {
        bool got_packet = strlen(cmd.in[0]) > 0;

        if (got_packet) {
            pd->wrapper = fake_packet(lcdata, cmd.in);
            pd->data    = rte_pktmbuf_mtod(pd->wrapper, uint8_t*);
        }

        if (cmd.sleep_millis > 0) {
            sleep_millis(cmd.sleep_millis);
        }

        return got_packet;
    }

    if (cmd.action == FAKE_SETDEF) {
        typedef void* (*ctrl_setdef)(const char*);

        ctrl_setdef setdef = (ctrl_setdef)(cmd.ptr);
        const char* arg = cmd.in[1];

        setdef(arg);
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
    if (get_cmd(lcdata).action == FAKE_PKT && got_packet)  ++lcdata->pkt_idx;

    ++lcdata->idx;
}

uint32_t get_portid(struct lcore_data* lcdata, unsigned queue_idx) {
    return get_cmd(lcdata).in_port;
}

void main_loop_rx_group(struct lcore_data* lcdata, unsigned queue_idx) {

}

unsigned get_pkt_count_in_group(struct lcore_data* lcdata) {
    return 1;
}

unsigned get_queue_count(struct lcore_data* lcdata) {
    return 1;
}

void send_single_packet(struct lcore_data* lcdata, packet_descriptor_t* pd, packet* pkt, int egress_port, int ingress_port, bool send_clone) {
    struct rte_mbuf* mbuf = (struct rte_mbuf *)pkt;
    dbg_bytes(rte_pktmbuf_mtod(mbuf, uint8_t*), rte_pktmbuf_pkt_len(mbuf),
              "Emitting " T4LIT(packet #%d,packet) "@" T4LIT(core%d,core) " on port " T4LIT(%d,port) " (" T4LIT(%d) " bytes): ",
              lcdata->pkt_idx + 1, rte_lcore_id(),
              egress_port, rte_pktmbuf_pkt_len(mbuf));

    check_sent_packet(lcdata, pd, egress_port, ingress_port);
}

bool storage_already_inited = false;

void init_storage() {
    if (storage_already_inited)    return;

    char str[15];
    sprintf(str, "testpool");
    pktmbuf_pool[0] = rte_mempool_create(str, (unsigned)1023, MBUF_SIZE, MEMPOOL_CACHE_SIZE, sizeof(struct rte_pktmbuf_pool_private), rte_pktmbuf_pool_init, NULL, rte_pktmbuf_init, NULL, 0, 0);

    storage_already_inited = true;
}

struct lcore_data init_lcore_data() {
    return (struct lcore_data) {
        .conf     = &lcore_conf[rte_lcore_id()],
        .is_valid = true,
        .idx      = 0,
        .pkt_idx  = 0,
        .mempool  = pktmbuf_pool[0],
    };
}

void initialize_nic() {
    dpdk_init_nic();
}

void t4p4s_abnormal_exit(int retval, int idx) {
    if (launch_count() == 1) {
        debug(T4LIT(Abnormal exit,error) ", code " T4LIT(%d) ".\n", retval);
    } else {
        debug(T4LIT(Abnormal exit,error) " in iteration " T4LIT(%d) "/" T4LIT(%d) ", code " T4LIT(%d) ".\n", idx, launch_count(), retval);
    }
}

void t4p4s_after_launch(int idx) {
    if (launch_count() == 1) {
        debug(T4LIT(Execution done.,success) "\n");
    } else {
        debug("Iteration" T4LIT(%d) "/" T4LIT(%d) " " T4LIT(done,success)".\n", idx, launch_count());
    }
}

void t4p4s_normal_exit() {
    debug(T4LIT(Normal exit.,success) "\n");
}

void t4p4s_post_launch(int idx) {

}


// TODO make this parameterizable
uint32_t get_port_mask() {
    return 0xF;
}

// TODO make this parameterizable
uint8_t get_port_count() {
    return __builtin_popcount(get_port_mask());
}
