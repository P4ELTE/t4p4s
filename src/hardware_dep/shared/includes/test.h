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
#define NO_OUTPUT -1

#define IPPROTOCOL_ICMP "01"
#define IPPROTOCOL_IPv4 "04"
#define IPPROTOCOL_TCP  "06"
#define IPPROTOCOL_UDP  "11"

#define UDP_PORT_GTPC  "084B" /* 2123 */
#define UDP_PORT_GTPU  "0868" /* 2152 */
#define UDP_PORT_VXLAN "12B5" /* 4789 */
#define UDP_DPORT_RLC  "FF4F" /* 65359 */
#define UDP_PORT_BUFFER "303A" /*12346*/

#define ARP_HTYPE_ETHERNET "0001"

#define cIPV4     "0800"
#define cARP      "0806"
#define cIPV6     "86dd"
#define cVLAN     "8100"
#define cBAAS     "abef"
#define cRLC      "0101"

#define ARP_HLEN_ETHERNET  "06"
#define ARP_PLEN_IPV4      "04"

#define hETH(dsteth, srceth, tag)                  dsteth, srceth, tag
#define hETH4(dsteth, srceth)                      dsteth, srceth, cIPV4
#define hETH6(dsteth, srceth)                      dsteth, srceth, cIPV6
#define hICMPv4(srcip, dstip)                      "050000000000000000", IPPROTOCOL_ICMP, "0000", srcip, dstip
#define hIPv4(srcip, dstip)                        "050000000000000000", IPPROTOCOL_IPv4, "0000", srcip, dstip
#define hTCPv4(srcip, dstip)                       "050000000000000000", IPPROTOCOL_TCP, "0000", srcip, dstip
#define hUDPv4(srcip, dstip)                       "050000000000000000", IPPROTOCOL_UDP, "0000", srcip, dstip
#define hIPv6(next_hdr, srcip, dstip)              "6" "00" "00000" "0000" next_hdr "00", srcip, dstip
#define hUDP(src_port, dst_port, length, checksum) src_port dst_port length checksum
// TODO more details/args
#define hARP(oper)                                     ARP_HTYPE_ETHERNET, cIPV4, ARP_HLEN_ETHERNET, ARP_PLEN_IPV4, oper
#define hVXLAN() "0000000000000000"
#define hBAAS() "0000000000000000"
#define hGTP() "0000000000000000"
#define hRLC() "00000000000000000000000000"
#define hPDCPbefore() "00000000000000000000000000000000000000000000000000000000000000000000"
#define hPDCP() "0000000000"

#define ETH(dsteth, srceth, ...)        FDATA(dsteth, srceth, cIPV4, ##__VA_ARGS__)
#define ETH6(dsteth, srceth, ...)       FDATA(dsteth, srceth, cIPV6, ##__VA_ARGS__)
#define ARP(dsteth, srceth, ...)        FDATA(dsteth, srceth, cARP, ##__VA_ARGS__)
#define VLAN(dsteth, srceth, ...)       FDATA(dsteth, srceth, cVLAN, ##__VA_ARGS__)

#define ETH_IPv4(dsteth, srceth, protocol, srcip, dstip, ...)    FDATA(hETH4(dsteth, srceth), hIPv4(protocol, srcip, dstip), ##__VA_ARGS__)
#define ETH_IPv6(dsteth, dstip6, srceth, srcip6, next_hdr, ...)  FDATA(hETH6(dsteth, srceth), hIPv6(next_hdr, srcip6, dstip6), ##__VA_ARGS__)

#define IPV4(dsteth, dstip, srceth, srcip, ...)                  FDATA(hETH4(dsteth, srceth), hIPv4(srcip, dstip), ##__VA_ARGS__)
#define ICMP(dsteth, dstip, srceth, srcip, ...)                  FDATA(hETH4(dsteth, srceth), hICMPv4(srcip, dstip), ##__VA_ARGS__)
#define UDP(dsteth, dstip, srceth, srcip, len, chksm, ...)       FDATA(hETH4(dsteth, srceth), hUDPv4(srcip, dstip), hUDP(srcip, dstip, len, chksm), ##__VA_ARGS__)

#define ARP_IPV4(dsteth, srceth, arp_oper, ...)   FDATA(hETH(dsteth, srceth, cARP), hARP(arp_oper), ##__VA_ARGS__)

#define VXLAN(dsteth, srceth, srcip, ...)               UDP(dsteth, UDP_PORT_VXLAN, srceth, srcip, "0000", "0000", ##__VA_ARGS__)
#define GTP(dsteth, srceth, srcip, ...)                 UDP(dsteth, UDP_PORT_GTPU, srceth, srcip, "0000", "0000", ##__VA_ARGS__)
#define GTPv1(dsteth, dstip, srceth, srcip, tFlag, ...) GTP(dsteth, dstip, srceth, srcip, "00000000", (tFlag?"28":"20"), ##__VA_ARGS__)
#define GTPv2(dsteth, dstip, srceth, srcip, tFlag, ...) GTP(dsteth, dstip, srceth, srcip, "00000000", (tFlag?"48":"40"), ##__VA_ARGS__)
#define PHYS_BUFFER(dsteth, srceth, srcip, ...)         UDP(dsteth, UDP_PORT_BUFFER, srceth, srcip, "0000", "0000", ##__VA_ARGS__)
#define RLC(dsteth, srceth, srcip, ...)                 UDP(dsteth, UDP_DPORT_RLC, srceth, srcip, "0000", "0000", ##__VA_ARGS__)


#define IPV6_0000 "00000000000000000000000000000000"

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
