// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include "testsuite.h"

// ------------------------------------------------------
// Testcase steps

#define FDATA(...)    { __VA_ARGS__, "" }

#define FSLEEP(time)  {FAKE_PKT, 0, 0, FDATA(""), time, 0, FDATA("")}
#define FEND          {FAKE_END, 0, 0, FDATA(""),    0, 0, FDATA("")}

// this packet is emitted unchanged
#define FCONSTANT(inport, outport, delay, pkt)  {FAKE_PKT, 0, inport, pkt, delay, outport, pkt}
// this packet is dropped
#define FDROPPED(inport, pkt)                   {FAKE_PKT, 0, inport, pkt, NO_CTL_REPLY, -1, FDATA("")}

#define UNKNOWN_PKT(src, dst, ...)        {FAKE_PKT, 0, 0, ETH(src, dst, ##__VA_ARGS__),  CTL_REPLIES,    0, ETH(src, dst, ##__VA_ARGS__)}
#define LEARNED_PKT(port, src, dst, ...)  {FAKE_PKT, 0, 0, ETH(src, dst, ##__VA_ARGS__), NO_CTL_REPLY, port, ETH(src, dst, ##__VA_ARGS__)}
#define NO_OUTPUT -1

// ------------------------------------------------------
// Tags

// IP protocols
#define cICMP "01"
#define cIPv4 "04"
#define cTCP  "06"
#define cUDP  "11"

#define UDP_PORT_GTPC   "084B" /* 2123 */
#define UDP_PORT_GTPU   "0868" /* 2152 */
#define UDP_PORT_VXLAN  "12B5" /* 4789 */
#define UDP_DPORT_RLC   "FF4F" /* 65359 */
#define UDP_PORT_BUFFER "303A" /* 12346 */

#define ARP_HTYPE_ETHERNET "0001"

#define cIPV4     "0800"
#define cARP      "0806"
#define cIPV6     "86dd"
#define cVLAN     "8100"
#define cBAAS     "abef"
#define cRLC      "0101"

#define ARP_HLEN_ETHERNET  "06"
#define ARP_PLEN_IPV4      "04"

// ------------------------------------------------------
// Timeouts

#define NO_CTL_REPLY 0
#define CTL_REPLIES 200

// ------------------------------------------------------
// Packet structure: header parts

#define hxxIPv4(vsn, ihl, tos, total_len, id, flags_and_offset, ttl, protocol, checksum)  vsn ihl tos total_len id flags_and_offset ttl protocol checksum
#define hxxIPv6(version, traffic_class, flow_label, payload_len, next_hdr, hop_limit)     version traffic_class flow_label payload_len next_hdr hop_limit

// bytes: [4b+4b+1+2+2+3b+13b+2], 2, 4
#define hxIPv4(protocol, checksum)                 hxxIPv4("0", "5", "00", "0000", "0000", "0000", "00", protocol, checksum)
// bytes: [4b+1+20b+4], 2, [2]
#define hxIPv6(next_hdr)                           hxxIPv6("6", "00", "00000", "0000", next_hdr, "00")

// ------------------------------------------------------
// Packet structure: headers

#define hETH(dsteth, srceth, tag)                  dsteth, srceth, tag
#define hETH4(dsteth, srceth)                      dsteth, srceth, cIPV4
#define hETH6(dsteth, srceth)                      dsteth, srceth, cIPV6
#define hICMPv4(srcip4, dstip4)                    hxIPv4(cICMP, "0000"), srcip4, dstip4
// bytes: [18], 4, 4
#define hIPv4(srcip4, dstip4)                      hxIPv4(cIPv4, "0000"), srcip4, dstip4
#define hTCPv4(srcip4, dstip4)                     hxIPv4(cTCP, "0000"), srcip4, dstip4
#define hUDPv4(srcip4, dstip4)                     hxIPv4(cUDP, "0000"), srcip4, dstip4
#define hIPv6(next_hdr, srcip6, dstip6)            hxIPv6(next_hdr), srcip6, dstip6
#define hUDP(src_port, dst_port, length, checksum) src_port dst_port length checksum
// bytes: [2+2+1+1], 2
#define hARP(oper)                                 ARP_HTYPE_ETHERNET, cIPV4, ARP_HLEN_ETHERNET, ARP_PLEN_IPV4, oper
#define hVXLAN()                                   "0000000000000000"
#define hBAAS()                                    "0000000000000000"
#define hGTP()                                     "0000000000000000"
#define hRLC()                                     "00000000000000000000000000"
#define hPDCPbefore()                              "00000000000000000000000000000000000000000000000000000000000000000000"
#define hPDCP()                                    "0000000000"

// bytes: 6, 6
#define ETH(dsteth, srceth, ...)        FDATA(hETH4(dsteth, srceth), ##__VA_ARGS__)
#define ETH6(dsteth, srceth, ...)       FDATA(hETH6(dsteth, srceth), ##__VA_ARGS__)
#define ARP(dsteth, srceth, ...)        FDATA(hETH(dsteth, srceth, cARP), ##__VA_ARGS__)
#define VLAN(dsteth, srceth, ...)       FDATA(hETH(dsteth, srceth, cVLAN), ##__VA_ARGS__)

#define IPV4(dsteth, dstip, srceth, srcip, ...)              ETH( dsteth, srceth, hIPv4(srcip, dstip), ##__VA_ARGS__)
#define ICMP(dsteth, dstip, srceth, srcip, ...)              ETH( dsteth, srceth, hICMPv4(srcip, dstip), ##__VA_ARGS__)
#define UDP(dsteth, dstip, srceth, srcip, len, chksm, ...)   ETH( dsteth, srceth, hUDPv4(srcip, dstip), hUDP(srcip, dstip, len, chksm), ##__VA_ARGS__)
#define IPV6(dsteth, dstip6, srceth, srcip6, next_hdr, ...)  ETH6(dsteth, srceth, hIPv6(next_hdr, srcip6, dstip6), ##__VA_ARGS__)

// bytes: 6, 6, [6], 2, 6, 4, 6, 4, payload
#define ARP_IPV4(dsteth, srceth, arp_oper, sha, spa, tha, tpa, ...)   ARP(dsteth, srceth, hARP(arp_oper), sha, spa, tha, tpa, ##__VA_ARGS__)

#define VXLAN(dsteth, srceth, srcip, ...)               UDP(dsteth, UDP_PORT_VXLAN, srceth, srcip, "0000", "0000", ##__VA_ARGS__)
#define GTP(dsteth, srceth, srcip, ...)                 UDP(dsteth, UDP_PORT_GTPU, srceth, srcip, "0000", "0000", ##__VA_ARGS__)
#define PHYS_BUFFER(dsteth, srceth, srcip, ...)         UDP(dsteth, UDP_PORT_BUFFER, srceth, srcip, "0000", "0000", ##__VA_ARGS__)
#define RLC(dsteth, srceth, srcip, ...)                 UDP(dsteth, UDP_DPORT_RLC, srceth, srcip, "0000", "0000", ##__VA_ARGS__)

#define GTPv1(dsteth, dstip, srceth, srcip, tFlag, ...) GTP(dsteth, dstip, srceth, srcip, "00000000", (tFlag?"28":"20"), ##__VA_ARGS__)
#define GTPv2(dsteth, dstip, srceth, srcip, tFlag, ...) GTP(dsteth, dstip, srceth, srcip, "00000000", (tFlag?"48":"40"), ##__VA_ARGS__)

// ------------------------------------------------------
// Constants

#define ZEROx8B "0000000000000000"
#define ZEROx6B "000000000000"
#define ZEROx4B "00000000"

#define IPV6_0000 "00000000000000000000000000000000"

#define IPV4_0000 "0000000000000000000000000000000000000000"
#define IPV4_FFFF "00000000000000000000ffff0000000000000000"

#define ETH00 "000000000000"
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
#define PAYLOAD13 ZEROx4B
#define PAYLOAD14 "f00ff00f"

#define PAYLOAD0x8B   ZEROx8B
#define PAYLOAD78x20B "7878787878787878787878787878787878787878"

// LPM prefixes

#define LPM1_TOP16B   "9600"
#define LPM2_TOP16B   "3200"
