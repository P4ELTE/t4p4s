// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include <netinet/ether.h>

#include "dpdk_lib.h"
#include "dpdk_nicoff.h"
#include "util_debug.h"
#include "dpdkx_crypto.h"
#include "util_packet.h"
#include "test.h"

#include "main.h"

extern int get_socketid(unsigned lcore_id);

extern struct lcore_conf lcore_conf[RTE_MAX_LCORE];
extern void dpdk_init_nic();

extern struct rte_mempool* pktmbuf_pool[NB_SOCKETS];

// ------------------------------------------------------

extern volatile int packet_counter;
extern volatile int packet_with_error_counter;

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
        #ifndef T4P4S_NO_CONTROL_PLANE
            // wait for "stray" control messages to go away
            if (idx != 0) {
                sleep_millis(PAUSE_BETWEEN_TESTCASES_MILLIS);
            }
        #endif

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


struct rte_mbuf* fake_packet(const char* texts[MAX_SECTION_COUNT], LCPARAMS) {
    int byte_count = total_fake_byte_count(texts);

    // debug("Creating fake " T4LIT(packet #%d,packet) " (" T4LIT(%d) " bytes)\n", lcdata->pkt_idx + 1, byte_count);
    struct rte_mbuf* p  = rte_pktmbuf_alloc(pktmbuf_pool[get_socketid(rte_lcore_id())]);
    uint8_t*         p2 = (uint8_t*)rte_pktmbuf_append(p, byte_count);

    if (p2 == NULL) {
        rte_exit(3, "Could not allocate space for fake packet\n");
    }

    while (strlen(*texts) > 0) {
        uint8_t* dst = p2;
        p2 = str2bytes(*texts, p2);

        // dbg_bytes(dst, strlen(*texts) / 2, " :::: " T4LIT(%2zd) " bytes: ", strlen(*texts) / 2);

        ++texts;
    }

    return p;
}

void abort_on_strict() {
#ifdef T4P4S_STRICT
    rte_exit(1, "Unexpected data, " T4LIT(aborting,error) "\n");
#endif
}

void check_egress_port(fake_cmd_t cmd, int egress_port, LCPARAMS) {
    if (cmd.out_port == egress_port)    return;

    char port_designation_egress[256];
    port_designation_egress[0] = '\0';

    char port_designation_cmd[256];
    port_designation_cmd[0] = '\0';

    if (egress_port == T4P4S_BROADCAST_PORT) {
        strcpy(port_designation_egress, " (broadcast)");
    }

    if (cmd.out_port == T4P4S_BROADCAST_PORT) {
        strcpy(port_designation_cmd, " (broadcast)");
    }

    debug("   " T4LIT(!!,error) " " T4LIT(packet #%d,packet) "@" T4LIT(core%d,core) ": expected egress port is " T4LIT(%d%s,expected) ", got " T4LIT(%d%s,error) "\n",
          lcdata->pkt_idx + 1, rte_lcore_id(),
          cmd.out_port, port_designation_cmd,
          egress_port, port_designation_egress);
    lcdata->is_valid = false;
    abort_on_strict();
}

bool check_byte_count(fake_cmd_t cmd, LCPARAMS) {
    if (pd->wrapper == 0) {
        rte_exit(1, "Error: packet was not created in memory, " T4LIT(aborting,error) "\n");
    }

    struct rte_mbuf* mbuf = (struct rte_mbuf*)pd->wrapper;

    int expected_byte_count = total_fake_byte_count(cmd.out);
    int actual_byte_count = rte_pktmbuf_pkt_len(mbuf);
    if (expected_byte_count != actual_byte_count) {
        debug("   " T4LIT(!!,error) " " T4LIT(packet #%d,packet) "@" T4LIT(core%d,core) ": expected " T4LIT(%d,expected) " bytes, got " T4LIT(%d,error) "\n",
              lcdata->pkt_idx + 1, rte_lcore_id(),
              expected_byte_count, actual_byte_count);

        lcdata->is_valid = false;
        abort_on_strict();
        return false;
    }
    return true;
}

int get_wrong_byte_count(fake_cmd_t cmd, LCPARAMS) {
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

void print_wrong_bytes(fake_cmd_t cmd, int wrong_byte_count, LCPARAMS) {
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
    debug("   " T4LIT(!!,error) " " T4LIT(%d) " wrong byte%s found: %s\n", wrong_byte_count, wrong_byte_count > 1 ? "s" : "", msg);
}

void print_expected_bytes(fake_cmd_t cmd, int wrong_byte_count, LCPARAMS) {
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
                written_bytes = sprintf(msgptr, T4LIT(%02x,success), expected_byte);
            } else {
                written_bytes = sprintf(msgptr, T4LIT(%02x,expected), expected_byte);
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
    char tmpmsg[MSG_MAX_LEN];
    char* tmpmsgptr = tmpmsg;
    sprintf(tmpmsgptr, "%d", wrong_byte_count);
    debug("   " T4LIT(!!,error) " Expected byte%s:     %*s%s\n", wrong_byte_count > 1 ? "s" : "", (int)strlen(tmpmsgptr), "", msg);
}

void check_packet_contents(fake_cmd_t cmd, LCPARAMS) {
    int wrong_byte_count = get_wrong_byte_count(cmd, LCPARAMS_IN);

    if (wrong_byte_count != 0) {
        print_wrong_bytes(cmd, wrong_byte_count, LCPARAMS_IN);
        print_expected_bytes(cmd, wrong_byte_count, LCPARAMS_IN);
        lcdata->is_valid = false;
        abort_on_strict();
    }
}

bool encountered_error = false;
bool encountered_drops = false;

fake_cmd_t get_cmd(int idx) {
    if(idx < 0){
        fake_cmd_t ret = {FAKE_PKT, 0, 0, FDATA(""), 0, 0, FDATA("")};
        return ret;
    }else{
        return (*(current_test_case->steps))[rte_lcore_id()][idx];
    }
}

bool is_real_fake_packet(fake_cmd_t cmd) {
    return cmd.action == FAKE_PKT && strlen(cmd.out[0]) != 0;
}

fake_cmd_t get_next_real_fake_verify_packet(LCPARAMS) {
    while(true){
        ++lcdata->verify_idx;
        fake_cmd_t cmd = get_cmd(lcdata->verify_idx);
        if(is_real_fake_packet(cmd) || cmd.action == FAKE_END){
            return cmd;
        }
    }
}

void check_sent_packet(int egress_port, int ingress_port, LCPARAMS) {
    fake_cmd_t cmd = get_next_real_fake_verify_packet(LCPARAMS_IN);
    check_egress_port(cmd, egress_port, LCPARAMS_IN);
    bool is_ok = check_byte_count(cmd, LCPARAMS_IN);
    if (is_ok) {
        check_packet_contents(cmd, LCPARAMS_IN);
    }

    #ifdef T4P4S_STATS

    if (cmd.require[0]>0 || cmd.forbid[0]>0) {
        bool requirements_ok = check_controlflow_requirements(cmd);

        if (requirements_ok) {
            debug( "   " T4LIT(<<,success) " " T4LIT(Packet #%d,packet) "@" T4LIT(core%d,core) " is " T4LIT(passing the control-flow requirements, success) "\n", lcdata->pkt_idx + 1, rte_lcore_id());
        } else {
            debug( "   " T4LIT(!!,error)" " T4LIT(Packet #%d,packet) "@" T4LIT(core%d,core) " is " T4LIT(failing the control-flow requirements, error) "\n", lcdata->pkt_idx + 1, rte_lcore_id());
            encountered_error = true;
        }
    }

    #endif

    if (lcdata->is_valid) {
        debug( "   " T4LIT(<<,success) " " T4LIT(Packet #%d,packet) "@" T4LIT(core%d,core) " is " T4LIT(sent successfully,success) "\n", lcdata->pkt_idx + 1, rte_lcore_id());
    } else {
        debug( "   " T4LIT(!!,error)" " T4LIT(Packet #%d,packet) "@" T4LIT(core%d,core) " is " T4LIT(sent with errors,error) "\n", lcdata->pkt_idx + 1, rte_lcore_id());
        encountered_error = true;
    }
}

// ------------------------------------------------------
// TODO

#ifdef START_CRYPTO_NODE
bool core_stopped_running[RTE_MAX_LCORE];
#endif

bool core_is_working(LCPARAMS) {
    #if defined ASYNC_MODE && ASYNC_MODE != ASYNC_MODE_OFF
        bool ret =
            get_cmd(lcdata->idx).action != FAKE_END ||
            lcdata->conf->pending_crypto > 0 ||
            rte_ring_count(lcdata->conf->async_queue) > 0;

        #ifdef START_CRYPTO_NODE
            if(ret == false){
                core_stopped_running[rte_lcore_id()] = true;
            }

            if (is_crypto_node()) {
                bool is_any_runing = false;
                for(int a = 0; a < crypto_node_id();a++){
                    is_any_runing |= !core_stopped_running[a];
                }
                ret = is_any_runing;
            }
        #endif
        return ret;
    #else
        return get_cmd(lcdata->idx).action != FAKE_END;
    #endif
}

extern volatile bool ctrl_is_initialized;

void await_ctl_init() {
    #ifndef T4P4S_NO_CONTROL_PLANE
        int MAX_CTL_INIT_MILLIS = 50;
        for (int i = 0; i < MAX_CTL_INIT_MILLIS; ++i) {
            if (ctrl_is_initialized)    break;
            sleep_millis(1);
        }
    #endif
}

bool receive_packet(unsigned pkt_idx, LCPARAMS) {
    if (pkt_idx == 0) {
        await_ctl_init();
    }

    lcdata->idx++;
    fake_cmd_t cmd = get_cmd(lcdata->idx);
    if (cmd.action == FAKE_PKT) {
        bool got_packet = strlen(cmd.in[0]) > 0;

        if (got_packet) {
            pd->wrapper = fake_packet(cmd.in, LCPARAMS_IN);
            pd->data    = rte_pktmbuf_mtod(pd->wrapper, uint8_t*);
        }

        #ifndef T4P4S_NO_CONTROL_PLANE
            // wait for answer from fake control plane
            if (cmd.sleep_millis > 0) {
                sleep_millis(cmd.sleep_millis);
            }
        #endif

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

int packet_expected_length(const char* sections[MAX_SECTION_COUNT]) {
    int retval = 0;
    for (int idx = 0; strlen(sections[idx]) != 0; ++idx) {
        retval += strlen(sections[idx]) / 2;
    }
    return retval;
}

void free_packet(LCPARAMS) {
    rte_free(pd->wrapper);

    if (get_cmd(lcdata->idx).out_port != -1) {
        ++packet_with_error_counter;
        encountered_drops = true;
        debug(" " T4LIT(!!!!,error) " Packet was supposed to be sent to " T4LIT(port %d,port) " with " T4LIT(%dB) " of data, but it was " T4LIT(dropped,error) "\n",
              get_cmd(lcdata->idx).out_port,
              packet_expected_length(get_cmd(lcdata->idx).out)
        );
    }
}

bool is_packet_handled(LCPARAMS) {
    return get_cmd(lcdata->idx).action == FAKE_PKT;
}

void main_loop_pre_rx(LCPARAMS) {
    #if defined T4P4S_STATS && T4P4S_STATS == 1
        t4p4s_init_per_packet_stats();
    #endif

    lcdata->is_valid = true;
}

void main_loop_post_rx(bool got_packet, LCPARAMS) {
    #if defined T4P4S_STATS && T4P4S_STATS == 1
        t4p4s_print_per_packet_stats();
    #endif

    if (got_packet) {
        ++packet_counter;
    }
}

void main_loop_post_single_rx(bool got_packet, LCPARAMS) {
    if (!lcdata->is_valid)    ++packet_with_error_counter;

    if (get_cmd(lcdata->idx).action == FAKE_PKT && got_packet)  ++lcdata->pkt_idx;
}

uint32_t get_portid(unsigned queue_idx, LCPARAMS) {
    return get_cmd(lcdata->idx).in_port;
}

void main_loop_rx_group(unsigned queue_idx, LCPARAMS) {

}

unsigned get_pkt_count_in_group(LCPARAMS) {
    if(get_cmd(lcdata->idx).action == FAKE_PKT)
        return 1;
    else
        return 0;
}

unsigned get_queue_count(LCPARAMS) {
    return 1;
}

void send_single_packet(packet* pkt, int egress_port, int ingress_port, bool send_clone, LCPARAMS) {
    struct rte_mbuf* mbuf = (struct rte_mbuf *)pkt;

    if (get_cmd(lcdata->idx).out_port == -1) {
        ++packet_with_error_counter;
        encountered_drops = true;
        debug(" " T4LIT(!!!!,error) " Packet was supposed to be " T4LIT(dropped,warning) ", but it was " T4LIT(sent,error) " to " T4LIT(port %d,port) " with " T4LIT(%dB) " of data\n",
              egress_port,
              packet_length(pd)
        );
        return;
    }

    check_sent_packet(egress_port, ingress_port, LCPARAMS_IN);
}

bool storage_already_inited = false;

// defined in main_async.c
void async_init_storage();

void init_storage() {
    if (storage_already_inited) return;
    pktmbuf_pool[0] = rte_mempool_create("main_pool", (unsigned)1023, MBUF_SIZE, MEMPOOL_CACHE_SIZE, sizeof(struct rte_pktmbuf_pool_private), rte_pktmbuf_pool_init, NULL, rte_pktmbuf_init, NULL, 0, 0);
    #if defined ASYNC_MODE && ASYNC_MODE != ASYNC_MODE_OFF
        async_init_storage();
    #endif

    storage_already_inited = true;

    #ifdef START_CRYPTO_NODE
        for(int a=0;a<rte_lcore_count();a++){
            core_stopped_running[a] = false;
        }
    #endif
}

extern void init_async_data(struct lcore_data *lcdata);

struct lcore_data init_lcore_data() {
    t4p4s_init_global_stats();
    struct lcore_data lcdata = (struct lcore_data) {
        .conf       = &lcore_conf[rte_lcore_id()],
        .is_valid   = true,
        .verify_idx = -1,
        .idx        = -1,
        .pkt_idx    = 0,
        .mempool    = pktmbuf_pool[0],
    };

    #if defined ASYNC_MODE && ASYNC_MODE != ASYNC_MODE_OFF
        init_async_data(&lcdata);
    #endif
    return lcdata;
}

void initialize_nic() {
    dpdk_init_nic();
    #if T4P4S_INIT_CRYPTO
        init_crypto_devices();
    #endif
}

void t4p4s_abnormal_exit(int retval, int idx) {
    t4p4s_print_global_stats();

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

int t4p4s_normal_exit() {
    t4p4s_print_global_stats();

    if (encountered_error) {
        debug(T4LIT(Normal exit,success) " but " T4LIT(errors in processing packets,error) "\n");
        return 3;
    }

    if (encountered_drops) {
        debug(T4LIT(Normal exit,success) " but " T4LIT(some packets were unexpectedly dropped/sent,error) "\n");
        return 4;
    }

    debug(T4LIT(Normal exit.,success) "\n");
    return 0;
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

int get_packet_idx(LCPARAMS) {
    return lcdata->pkt_idx + 1;
}
