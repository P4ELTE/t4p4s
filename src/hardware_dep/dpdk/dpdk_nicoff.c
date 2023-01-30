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

bool is_valid_fake_packet(const char* texts[MAX_SECTION_COUNT], LCPARAMS);

extern struct rte_mempool* pktmbuf_pool[NB_SOCKETS];

// ------------------------------------------------------

const int NO_INFINITE_LOOP = -1;

int last_error_pkt_idx = -1;
bool encountered_error = false;
bool encountered_drops = false;
bool encountered_bad_requirement = false;
bool encountered_unset_egress_port = false;
int infinite_loop_on_core = NO_INFINITE_LOOP;

void error_encountered(LCPARAMS) {
    encountered_error = true;
    last_error_pkt_idx = lcdata->pkt_idx;
}

// ------------------------------------------------------

extern volatile int packet_counter;
extern volatile int packet_with_error_counter;

// ------------------------------------------------------

const int REASONABLE_ITER_LIMIT = 100;

// ------------------------------------------------------
// Exports

uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;

// ------------------------------------------------------
// Test cases

const char* fake_cmd_e_strings[] = {"FAKE_PKT", "FAKE_SETDEF", "FAKE_END"};

#define FAKE_CMD_ACTION_TO_STR(cmd) (fake_cmd_e_strings[(int)cmd])


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

int get_packet_idx(LCPARAMS) {
    return lcdata->pkt_idx + 1;
}

bool is_final_section(const char*const text) {
    return (text == NULL) || strlen(text) == 0;
}

bool starts_with_fmt_char(const char*const text) {
    return text[0] == '\0' || text[0] == '<' || text[0] == '|' || text[0] == '>';
}

// The input may contain <..in..|..out..> parts.
// Only the part indicated by is_in is kept, the other chars are skipped over.
const char* skip_chars(const char*const text, bool is_in) {
    const char* ptr = text;
    char start, end, skip1;

    if (is_in) {
        start = '|';
        end   = '>';
        skip1 = '<';
    } else {
        start = '<';
        end   = '|';
        skip1 = '>';
    }

    char tmp[128];

    if (ptr[0] == skip1) {
        ++ptr;
    }

    if (ptr[0] == start) {
        // skip over <.......| or |......> including the end character
        ptr = strchr(ptr, end) + 1;
    }

    return ptr;
}

int packet_len(const char* texts[MAX_SECTION_COUNT], bool is_in) {
    int byte_count = 0;

    for (; !is_final_section(*texts); ++texts) {
        const char* text = *texts;
        while (text[0] != '\0') {
            text = skip_chars(text, is_in);
            if (starts_with_fmt_char(text))   continue;

            ++byte_count;
            text += 2;
        }
    }
    return byte_count;
}


uint8_t bytes[8*sizeof(struct ether_header)];

// str is a string that contains hex numbers without spaces.
// Their values are copied as bytes into dst in a linear fashion,
// and the pointer after the last written position is retured.
static uint8_t* str2bytes(const char* str, uint8_t* dst)
{
    const char* pos = str;
    while (*pos != 0) {
        const char* prev_pos = pos;
        pos = skip_chars(pos, true);
        if (prev_pos != pos) continue;

        sscanf(pos, "%2hhx", dst);
        pos += 2;
        ++dst;
    }

    return dst;
}


#define MAX_PACKET_SIZE 1024
uint8_t tmp[MAX_PACKET_SIZE];

struct rte_mbuf* fake_packet(const char* texts[MAX_SECTION_COUNT], LCPARAMS) {
    int byte_count = packet_len(texts, true);

    struct rte_mbuf* p  = rte_pktmbuf_alloc(pktmbuf_pool[get_socketid(rte_lcore_id())]);
    uint8_t*         p2 = (uint8_t*)rte_pktmbuf_append(p, byte_count);

    if (p2 == NULL) {
        rte_exit(3, "Could not allocate space for fake packet\n");
    }

    while (!is_final_section(*texts)) {
        p2 = str2bytes(*texts, p2);

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
    #ifdef T4P4S_DEBUG
        if (!pd->is_egress_port_set && !is_packet_dropped(pd)) {
            debug(" " T4LIT(!!!!,error) " Egress port is not set for packet, nor is the packet dropped\n");
            encountered_unset_egress_port = true;
            return;
        }
    #endif

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

    if (lcdata->pkt_idx != last_error_pkt_idx) {
        if (cmd.out_port == DROP) {
            debug("   " T4LIT(!!,error) " " T4LIT(Packet #%d,packet) "@" T4LIT(core%d,core) ": expected to drop the packet, but got egress port " T4LIT(%d%s,error) "\n",
                get_packet_idx(LCPARAMS_IN), rte_lcore_id(),
                egress_port, port_designation_egress);
        } else {
            debug("   " T4LIT(!!,error) " " T4LIT(Packet #%d,packet) "@" T4LIT(core%d,core) ": expected egress port is " T4LIT(%d%s,port) ", got " T4LIT(%d%s,error) "\n",
                get_packet_idx(LCPARAMS_IN), rte_lcore_id(),
                cmd.out_port, port_designation_cmd,
                egress_port, port_designation_egress);
        }
        error_encountered(LCPARAMS_IN);
    }

    lcdata->is_valid = false;
    abort_on_strict();
}

bool check_byte_count(fake_cmd_t cmd, LCPARAMS) {
    if (pd->wrapper == 0) {
        rte_exit(1, "Error: packet was not created in memory, " T4LIT(aborting,error) "\n");
    }

    struct rte_mbuf* mbuf = (struct rte_mbuf*)pd->wrapper;

    int expected_byte_count = packet_len(cmd.out, false);
    int actual_byte_count = rte_pktmbuf_pkt_len(mbuf);
    if (expected_byte_count != actual_byte_count) {
        debug("   " T4LIT(!!,error) " " T4LIT(Packet #%d,packet) "@" T4LIT(core%d,core) ": expected " T4LIT(%d,expected) " bytes, got " T4LIT(%d,error) "\n",
              get_packet_idx(LCPARAMS_IN), rte_lcore_id(),
              expected_byte_count, actual_byte_count);

        lcdata->is_valid = false;
        abort_on_strict();
        return false;
    }
    return true;
}


#define MSG_MAX_LEN 4096

void write_txt(const char*const txt, char** expected, char** wrong) {
    if (*expected != NULL)    *expected += sprintf(*expected, "%s", txt);
    if (*wrong != NULL)       *wrong    += sprintf(*wrong,    "%s", txt);
}

int wrong_bytes_info(fake_cmd_t cmd, char* expected, char* wrong, LCPARAMS) {
    int wrong_byte_count = 0;

    int byte_idx = 0;
    for (const char** texts = cmd.out; !is_final_section(*texts); ++texts) {
        const char* text = *texts;

        write_txt("[", &expected, &wrong);

        while (text[0] != '\0') {
            text = skip_chars(text, false);
            if (text[0] == '\0')   break;

            if (starts_with_fmt_char(text))   continue;

            uint8_t expected_byte;
            sscanf(text, "%2hhx", &expected_byte);

            uint8_t actual_byte = pd->data[byte_idx];
            if (expected_byte != actual_byte) {
                wrong    += sprintf(wrong, T4LIT(%02x,error), actual_byte);
                expected += sprintf(expected, T4LIT(%02x,expected), expected_byte);
                ++wrong_byte_count;
            } else {
                wrong    += sprintf(wrong, T4LIT(%02x,expected), actual_byte);
                expected += sprintf(expected, T4LIT(%02x,expected), expected_byte);
            }

            ++byte_idx;
            text += 2;
        }

        write_txt("]", &expected, &wrong);
    }

    write_txt("\0", &expected, &wrong);

    return wrong_byte_count;
}


int last_printed_wrong_byte_msg_idx = -1;

void print_wrong_bytes_msg(fake_cmd_t cmd, char expected[MSG_MAX_LEN], char wrong[MSG_MAX_LEN], int wrong_byte_count, LCPARAMS) {
    if (last_printed_wrong_byte_msg_idx == lcdata->idx)    return;

    // this is padding for the second line
    char wronglen[MSG_MAX_LEN];
    sprintf(wronglen, "%d", wrong_byte_count);

    debug("   " T4LIT(!!,error) " " T4LIT(%d) " wrong byte%s found: %s\n", wrong_byte_count, wrong_byte_count > 1 ? "s" : "", wrong);
    debug("   " T4LIT(!!,error) " Expected byte%s:     %*s%s\n", wrong_byte_count > 1 ? "s" : "", (int)strlen(wronglen), "", expected);

    last_printed_wrong_byte_msg_idx = lcdata->idx;
}

void check_packet_contents(fake_cmd_t cmd, LCPARAMS) {
    char expected[MSG_MAX_LEN];
    char wrong[MSG_MAX_LEN];

    int wrong_byte_count = wrong_bytes_info(cmd, expected, wrong, LCPARAMS_IN);

    if (wrong_byte_count != 0) {
        print_wrong_bytes_msg(cmd, expected, wrong, wrong_byte_count, LCPARAMS_IN);
        lcdata->is_valid = false;
        abort_on_strict();
    }
}

#define SPECIAL_PORT_DESIGNATOR_COUNT 2
int special_port_designators[] = { T4P4S_BROADCAST_PORT, EGRESS_DROP_VALUE };

#define RND_PORT_COUNT 4
int rnd_port_designators[] = { RND1, RND2, RND3, RND4 };
int rnd_ports[4];

#define NO_INDEX_FOUND -1

int find_index_in_array(int value, int array[], int len) {
    for (int i = 0; i < len; ++i) {
        if (array[i] == value)   return i;
    }
    return NO_INDEX_FOUND;
}

void generate_random_port(fake_cmd_t* cmd) {
    cmd->in_port = pick_random_port();
    while (NO_INDEX_FOUND != find_index_in_array(cmd->in_port, special_port_designators, SPECIAL_PORT_DESIGNATOR_COUNT)) {
        cmd->in_port = pick_random_port();
    }
}

fake_cmd_t get_cmd(int idx) {
    if (idx < 0) {
        fake_cmd_t ret = {FAKE_PKT, 0, 0, FDATA(""), 0, 0, FDATA(""), ""};
        return ret;
    }

    fake_cmd_t* cmd = &(*(current_test_case->steps))[rte_lcore_id()][idx];

    if (cmd->action == FAKE_PKT) {
        if (cmd->in_port == ANY) {
            generate_random_port(cmd);
        }

        if (cmd->out_port == SAME) {
            cmd->out_port = cmd->in_port;
        }

        int in_idx = find_index_in_array(cmd->in_port, rnd_port_designators, RND_PORT_COUNT);
        int out_idx = find_index_in_array(cmd->out_port, rnd_port_designators, RND_PORT_COUNT);
        if (NO_INDEX_FOUND != in_idx)     cmd->in_port = rnd_ports[in_idx];
        if (NO_INDEX_FOUND != out_idx)    cmd->out_port = rnd_ports[out_idx];
    }


    return *cmd;
}

bool is_real_fake_packet(fake_cmd_t cmd) {
    return cmd.action == FAKE_PKT && strlen(cmd.out[0]) != 0;
}

fake_cmd_t get_next_real_fake_verify_packet(bool is_broadcast_nonfirst, LCPARAMS) {
    if (is_broadcast_nonfirst)    return get_cmd(lcdata->verify_idx);

    while (true) {
        ++lcdata->verify_idx;
        fake_cmd_t cmd = get_cmd(lcdata->verify_idx);
        if (is_real_fake_packet(cmd) || cmd.action == FAKE_END) {
            return cmd;
        }
    }
}

bool check_packet_after_parse(LCPARAMS) {
    fake_cmd_t cmd = get_cmd(lcdata->idx);
    // TODO make the PAYLOAD(...) macro mark the payload in a recognisable manner
    // if (pd->payload_size != cmd->payload_size) {
    //     // TODO warning message about differing payload sizes
    //     return false;
    // }
    return true;
}

void check_cflow_reqs(fake_cmd_t cmd, bool is_broadcast_nonfirst, LCPARAMS) {
    #ifdef T4P4S_STATS
        if (!is_broadcast_nonfirst && cmd.requirements[0] > 0) {
            bool requirements_ok = check_controlflow_requirements(cmd);

            if (requirements_ok) {
                debug( "   " T4LIT(::,success) " " T4LIT(Packet #%d,packet) "@" T4LIT(core%d,core) " " T4LIT(passed,success) " all control flow requirements\n", get_packet_idx(LCPARAMS_IN), rte_lcore_id());
            } else {
                if (lcdata->pkt_idx != last_error_pkt_idx) {
                    debug( "   " T4LIT(!!,error)" " T4LIT(Packet #%d,packet) "@" T4LIT(core%d,core) " " T4LIT(failed,error) " some control flow requirements\n", get_packet_idx(LCPARAMS_IN), rte_lcore_id());
                }
                encountered_bad_requirement = true;
            }
        }
    #endif
}

void check_sent_packet(int egress_port, int ingress_port, bool is_broadcast_nonfirst, LCPARAMS) {
    fake_cmd_t cmd = get_next_real_fake_verify_packet(is_broadcast_nonfirst, LCPARAMS_IN);
    check_egress_port(cmd, egress_port, LCPARAMS_IN);
    bool is_ok = check_byte_count(cmd, LCPARAMS_IN);
    if (is_ok) {
        check_packet_contents(cmd, LCPARAMS_IN);
        check_cflow_reqs(cmd, is_broadcast_nonfirst, LCPARAMS_IN);
    }

    if (!lcdata->is_valid) {
        if (lcdata->pkt_idx != last_error_pkt_idx) {
            debug("   " T4LIT(!!,error)" " T4LIT(Packet #%d,packet) "@" T4LIT(core%d,core) " is " T4LIT(sent with errors,error) "\n", get_packet_idx(LCPARAMS_IN), rte_lcore_id());
        }
        error_encountered(LCPARAMS_IN);
    }
}

#ifdef START_CRYPTO_NODE
bool core_stopped_running[RTE_MAX_LCORE];
#endif

bool is_over_iteration_limit(LCPARAMS) {
    ++lcdata->iter_idx;

    if (lcdata->iter_idx > REASONABLE_ITER_LIMIT) {
        debug( "   " T4LIT(!!,error) " Detected " T4LIT(%d iterations,error) " on " T4LIT(core %d) ", possible infinite loop\n", lcdata->iter_idx, rte_lcore_id());
        error_encountered(LCPARAMS_IN);
        infinite_loop_on_core = rte_lcore_id();
        return true;
    }

    return false;
}

#ifdef START_CRYPTO_NODE
    int crypto_node_id();
    bool is_crypto_node();
#endif

bool core_is_working(LCPARAMS) {
    #ifdef T4P4S_DEBUG
        if (is_over_iteration_limit(LCPARAMS_IN)) {
            return false;
        }
    #endif

    #if defined ASYNC_MODE && ASYNC_MODE != ASYNC_MODE_OFF
        unsigned in_async_queue_real = rte_ring_count(lcdata->conf->async_queue);

        if(in_async_queue_real != lcdata->conf->pending_in_async_queue){
            debug( "   " T4LIT(!!,error) " pending_in_async_queue is not valid, pending_in_async_queue=%d, real=%d!\n", lcdata->conf->pending_in_async_queue, in_async_queue_real);
            error_encountered(LCPARAMS_IN);
        }

        bool ret =
            get_cmd(lcdata->idx).action != FAKE_END ||
            lcdata->conf->pending_crypto > 0 ||
            lcdata->conf->pending_in_async_queue > 0;

        #ifdef START_CRYPTO_NODE
            if(ret == false){
                core_stopped_running[rte_lcore_id()] = true;
            }

            if (is_crypto_node()) {
                bool is_any_running = false;
                for(int a = 0; a < crypto_node_id();a++){
                    is_any_running |= !core_stopped_running[a];
                }
                ret = is_any_running;
            }

            sleep_millis(100);
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

void debug_actual_cmd(const char* message, LCPARAMS){
    fake_cmd_t cmd = get_cmd(lcdata->idx);
    struct rte_mbuf* temp = fake_packet(cmd.in, LCPARAMS_IN);
    int length = rte_pktmbuf_pkt_len(temp);
    dbg_bytes(rte_pktmbuf_mtod(temp, uint8_t*), length,"%s idx:%d type:%s (" T4LIT(%d) " bytes): ", message,(int)lcdata->idx,FAKE_CMD_ACTION_TO_STR(cmd.action),length);
    rte_pktmbuf_free(temp);
}

bool receive_packet(unsigned pkt_idx, LCPARAMS) {
    if (pkt_idx == 0) {
        await_ctl_init();
    }

    lcdata->idx++;
    fake_cmd_t cmd = get_cmd(lcdata->idx);

    if (cmd.action == FAKE_PKT) {
        bool got_packet = strlen(cmd.in[0]) > 0;

        if (!is_valid_fake_packet(cmd.out, LCPARAMS_IN)) {
            lcdata->is_valid = false;
            abort_on_strict();
            return false;
        }

        if (got_packet) {
            pd->wrapper = fake_packet(cmd.in, LCPARAMS_IN);
            pd->data    = rte_pktmbuf_mtod(pd->wrapper, uint8_t*);
            ++lcdata->pkt_idx;
        }

        #ifndef T4P4S_NO_CONTROL_PLANE
            // wait for answer from fake control plane
            if (cmd.sleep_millis > 0) {
                sleep_millis(cmd.sleep_millis);
            }
        #endif

        return got_packet;
    }

    return false;
}

void free_packet(LCPARAMS) {
    rte_free(pd->wrapper);

    if (get_cmd(lcdata->idx).out_port == DROP) {
        debug(" " T4LIT(xxxx,status) " Packet was " T4LIT(dropped,status) " as expected\n");
    } else {
        ++packet_with_error_counter;
        encountered_drops = true;
        debug(" " T4LIT(!!!!,error) " Packet was supposed to be sent to " T4LIT(port %d,port) " with " T4LIT(%dB) " of data, but it was " T4LIT(dropped,error) "\n",
              get_cmd(lcdata->idx).out_port,
              packet_len(get_cmd(lcdata->idx).out, false)
        );
    }
}

bool is_packet_handled(LCPARAMS) {
    return get_cmd(lcdata->idx).action == FAKE_PKT;
}

void init_rnd_ports() {
    for (int i = 0; i < RND_PORT_COUNT; ++i) {
        rnd_ports[i] = pick_random_port();
    }
}

void main_loop_pre_rx(LCPARAMS) {
    #if defined T4P4S_STATS && T4P4S_STATS == 1
        t4p4s_init_per_packet_stats();
    #endif

    init_rnd_ports();

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

#if defined ASYNC_MODE && ASYNC_MODE != ASYNC_MODE_OFF
    void main_loop_pre_single_rx_async(LCPARAMS);
    void main_loop_post_single_rx_async(bool got_packet, LCPARAMS);
    void main_loop_pre_single_tx_async(LCPARAMS);
    void main_loop_post_single_tx_async(LCPARAMS);
#endif

void main_loop_pre_single_rx(LCPARAMS){
    #if defined ASYNC_MODE && ASYNC_MODE != ASYNC_MODE_OFF
        main_loop_pre_single_rx_async(LCPARAMS_IN);
    #endif
}


void main_loop_post_single_rx(bool got_packet, LCPARAMS) {
    #if defined ASYNC_MODE && ASYNC_MODE != ASYNC_MODE_OFF
        main_loop_post_single_rx_async(got_packet, LCPARAMS_IN);
    #endif
}

void main_loop_pre_single_tx(LCPARAMS){
    #if defined ASYNC_MODE && ASYNC_MODE != ASYNC_MODE_OFF
        main_loop_pre_single_tx_async(LCPARAMS_IN);
    #endif
}
void main_loop_post_single_tx(LCPARAMS){
    if (!lcdata->is_valid)    ++packet_with_error_counter;

    #if defined ASYNC_MODE && ASYNC_MODE != ASYNC_MODE_OFF
        main_loop_post_single_tx_async(LCPARAMS_IN);
    #endif

}

uint32_t get_portid(unsigned queue_idx, LCPARAMS) {
    return get_cmd(lcdata->idx).in_port;
}

void main_loop_rx_group(unsigned queue_idx, LCPARAMS) {

}

unsigned get_pkt_count_in_group(LCPARAMS) {
    if (get_cmd(lcdata->idx).action == FAKE_END)   return 0;

    return 1;
}

unsigned get_queue_count(LCPARAMS) {
    return 1;
}

void send_single_packet(packet* pkt, int egress_port, int ingress_port, bool is_broadcast_nonfirst, LCPARAMS) {
    struct rte_mbuf* mbuf = (struct rte_mbuf *)pkt;

    if (get_cmd(lcdata->idx).out_port == -1) {
        ++packet_with_error_counter;
        encountered_drops = true;
        debug(" " T4LIT(!!!!,error) " Packet was supposed to be " T4LIT(dropped,warning) ", but it was " T4LIT(sent,error) " to " T4LIT(port %d,port) " with " T4LIT(%dB) " of data\n",
              egress_port,
              packet_size(pd)
        );
        return;
    }

    check_sent_packet(egress_port, ingress_port, is_broadcast_nonfirst, LCPARAMS_IN);
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
extern void init_crypto_data(struct lcore_data *data);

struct lcore_data init_lcore_data() {
    t4p4s_init_global_stats();
    struct lcore_data lcdata = (struct lcore_data) {
        .conf       = &lcore_conf[rte_lcore_id()],
        .is_valid   = true,
        .verify_idx = -1,
        .idx        = -1,
        .pkt_idx    = 0,
        .iter_idx   = 0,
        .mempool    = pktmbuf_pool[0],
    };

    #if defined ASYNC_MODE && ASYNC_MODE != ASYNC_MODE_OFF
        init_async_data(&lcdata);
    #endif
    #if T4P4S_INIT_CRYPTO
        init_crypto_data(&lcdata);
    #endif
    return lcdata;
}

void initialize_nic() {
    srand(time(0));

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
        debug("Iteration " T4LIT(%d) "/" T4LIT(%d) " " T4LIT(done,success)".\n", idx, launch_count());
    }
}

int t4p4s_normal_exit() {
    t4p4s_print_global_stats();

    if (infinite_loop_on_core != NO_INFINITE_LOOP) {
        debug(T4LIT(Abnormal exit,error) ", too many iterations (" T4LIT(%d) " on lcore " T4LIT(%d,core) "), possible infinite loop.\n", REASONABLE_ITER_LIMIT, infinite_loop_on_core);
        return T4EXIT(LOOP);
    }

    if (encountered_error) {
        debug(T4LIT(Normal exit,success) " but " T4LIT(errors in processing packets,error) "\n");
        return T4EXIT(WRONG_OUTPUT);
    }

    if (encountered_drops) {
        debug(T4LIT(Normal exit,success) " but " T4LIT(some packets were unexpectedly dropped/sent,error) "\n");
        return T4EXIT(DROP_SEND);
    }

    #ifdef T4P4S_STATS
        if (encountered_bad_requirement) {
            debug(T4LIT(Normal exit,success) " but " T4LIT(some control flow requirements were not met,error) "\n");
            return T4EXIT(CFLOW_REQ);
        }
    #endif

    if (encountered_unset_egress_port) {
        debug(T4LIT(Normal exit,success) " but " T4LIT(packets were sent without an explicitly set egress port,error) "\n");
        return T4EXIT(UNSET_EGRESS_PORT);
    }

    debug(T4LIT(Normal exit.,success) "\n");
    return T4EXIT(OK);
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
