// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include <stdint.h>

// ------------------------------------------------------
// Fake packet data

enum fake_cmd_e {
    FAKE_PKT,
    // set default action
    FAKE_SETDEF,
    FAKE_END,
};

#define MAX_SECTION_COUNT 128

typedef struct fake_cmd_s {
    enum fake_cmd_e action;
    void*           ptr;
    uint32_t        in_port;
    const char*     in[MAX_SECTION_COUNT];

    int             sleep_millis;

    uint32_t        out_port;
    const char*     out[MAX_SECTION_COUNT];
} fake_cmd_t;

// ------------------------------------------------------
// Test suite

typedef struct testcase_s {
    const char* name;
    fake_cmd_t (*steps)[][RTE_MAX_LCORE];
} testcase_t;

#define TEST_SUITE_END { "", 0 }

#define MAX_TESTCASES 128

#define PAUSE_BETWEEN_TESTCASES_MILLIS 500

// ------------------------------------------------------
// Fake packet data creation helpers

#define INIT_WAIT_CONTROLPLANE_SHORT_MILLIS 500
#define INIT_WAIT_CONTROLPLANE_LONG_MILLIS  2000
#define WAIT_OTHER_CORE_PROCESSES_PACKAGES_MILLIS  500
#define WAIT_CONTROLPLANE_REPLY  200


#define FDATA(...)    { __VA_ARGS__, "" }

#define FSLEEP(time)  {FAKE_PKT, 0, 0, FDATA(""), time, 0, FDATA("")}
#define FEND          {FAKE_END, 0, 0, FDATA(""),    0, 0, FDATA("")}

#define UNKNOWN_PKT(src, dst, ...)        {FAKE_PKT, 0, 0, ETH(src, dst, ##__VA_ARGS__), WAIT_CONTROLPLANE_REPLY,    0, ETH(src, dst, ##__VA_ARGS__)}
#define LEARNED_PKT(port, src, dst, ...)  {FAKE_PKT, 0, 0, ETH(src, dst, ##__VA_ARGS__),                       0, port, ETH(src, dst, ##__VA_ARGS__)}

// Already defined in DPDK

//#define ETHERTYPE_IPV4 "0800"
//#define ETHERTYPE_ARP "0806"
//#define ETHERTYPE_VLAN "8100"

//#define IPPROTO_ICMP "01"
//#define IPPROTO_IPv4 "04"
//#define IPPROTO_TCP "06"
//#define IPPROTO_UDP "11"

//#define GTP_UDP_PORT "2152"

#define ARP_HTYPE_ETHERNET "0001"
#define ARP_PTYPE_IPV4     "0800"
#define ARP_HLEN_ETHERNET  "06"
#define ARP_PLEN_IPV4      "04"

#define ETH(dst, src, ...) FDATA(dst, src, ARP_PTYPE_IPV4, ##__VA_ARGS__)
#define IPV4(dsteth, dstip, srceth, srcip, ...) ETH(dsteth, srceth, ETHERTYPE_IPV4, "000000000000000000000000", srcip, dstip, ##__VA_ARGS__)
#define ICMP(dsteth, dstip, srceth, srcip, ...) ETH(dsteth, srceth, ETHERTYPE_IPV4, "000000000000000000", IPPROTO_ICMP, "0000", srcip, dstip, ##__VA_ARGS__)
#define UDP(dsteth, dstip, dstport, srceth, srcip, srcport, ...) ETH(dsteth, srceth, ETHERTYPE_IPV4, "000000000000000000", IPPROTO_UDP, "0000", srcip, dstip, srcport, dstport, ##__VA_ARGS__)
#define GTP(dsteth, dstip, srceth, srcip, ...) ETH(dsteth, srceth, ETHERTYPE_IPV4, "000000000000000000", IPPROTO_UDP, "0000", srcip, dstip, GTP_UDP_PORT, GTP_UDP_PORT, ##__VA_ARGS__)
#define GTPv1(dsteth, dstip, srceth, srcip, tFlag, ...) ETH(dsteth, srceth, ETHERTYPE_IPV4, "000000000000000000", IPPROTO_UDP, "0000", srcip, dstip, GTP_UDP_PORT, GTP_UDP_PORT, "00000000", "2", (tFlag?"8":"0"), ##__VA_ARGS__)
#define GTPv2(dsteth, dstip, srceth, srcip, tFlag, ...) ETH(dsteth, srceth, ETHERTYPE_IPV4, "000000000000000000", IPPROTO_UDP, "0000", srcip, dstip, GTP_UDP_PORT, GTP_UDP_PORT, "00000000", "4", (tFlag?"8":"0"), ##__VA_ARGS__)

#define ARP(dsteth, srceth, ...) ETH(dsteth, srceth, ETHERTYPE_ARP, ##__VA_ARGS__)
#define ARP_IPV4(dsteth, srceth, ...) ETH(dsteth, srceth, ETHERTYPE_ARP, ARP_HTYPE_ETHERNET, ARP_PTYPE_IPV4, ARP_HLEN_ETHERNET, ARP_PLEN_IPV4, ##__VA_ARGS__)

#define VLAN(dsteth, srceth, ...) ETH(dsteth, srceth, ETHERTYPE_VLAN, ##__VA_ARGS__)



#define IPV4_0000 "0000000000000000000000000000000000000000"
#define IPV4_FFFF "00000000000000000000ffff0000000000000000"

#define ETH01 "000001000000"
#define ETH02 "000002000000"
#define ETH03 "000003000000"
#define ETH04 "000004000000"

#define ETH1A "001234567890"
#define ETH1B "001234567891"

#define LPM_ETH1 "cccccccc0000"
#define LPM_ETH2 "dddddddd0000"

// other MAC addresses

#define L3_MAC1 "D2690FA8399C"
#define L3_MAC2 "D2690F00009C"

// random payloads

#define PAYLOAD01 "0123456789abcdef"
#define PAYLOAD02 "089789755756"
#define PAYLOAD03 "048989520487"
#define PAYLOAD04 "ffffffffffffff"

#define PAYLOAD11 "0a0a0a0a0a"
#define PAYLOAD12 "a0a0a0a0a0"
#define PAYLOAD13 "00000000"
#define PAYLOAD14 "f00ff00f"

// LPM prefixes

#define LPM1_TOP16B   "9600"
#define LPM2_TOP16B   "3200"

#define T4P4S_BROADCAST_PORT 100
