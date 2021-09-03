// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include "test_testsuite.h"

// ------------------------------------------------------
// Tags

// IP protocols
#define cICMP "01"
#define cIPv4 "04"
#define cTCP  "06"
#define cUDP  "11"

// UDP ports
#define pGTPC   "084B" /* 2123 */
#define pGTPU   "0868" /* 2152 */
#define pVXLAN  "12B5" /* 4789 */
#define pRLC    "FF4F" /* 65359 */
#define pBUF    "303A" /* 12346 */
#define p12345  "3039" /* 12345 */

#define ARP_HTYPE_ETHERNET "0001"

#define cIPV4     "0800"
#define cARP      "0806"
#define cIPV6     "86dd"
#define cVLAN     "8100"
#define cBAAS     "abef"
#define cRLC      "0101"
#define cLLDP     "88cc"

#define ARP_HLEN_ETHERNET  "06"
#define ARP_PLEN_IPV4      "04"

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

#define PORT0   c0x2B
#define LEN0    c0x2B
#define CHKSM0  c0x2B

#define hETH(dsteth, srceth, tag)                  dsteth srceth tag
#define hETH4(dsteth, srceth)                      dsteth srceth cIPV4
#define hETH6(dsteth, srceth)                      dsteth srceth cIPV6

//                                                 12     1         [1]     4       4
#define hIP4(protocol, srcip4, dstip4)             hxIPv4(protocol, CHKSM0) srcip4 dstip4
//  2       2       4     4     4b         4b  1     2      2        2
#define hTCP(srcPort,dstPort,seqNo,ackNo,dataOffset,res,flags,window,checksum,urgentPtr)   srcPort dstPort seqNo ackNo dataOffset res flags window checksum urgentPtr

#define hIP6(next_hdr, srcip6, dstip6)             hxIPv6(next_hdr)      srcip6 dstip6
//           2         2         2    2
#define hUDP(src_port, dst_port, len, chksm)       src_port dst_port len chksm
//                                                 4
#define hTEID(teid)                                teid

// the name only makes it easy to identify the header
#define hMISC(name, content)                       content


// bytes: [2+2+1+1], 2
#define hARP(oper)                                 ARP_HTYPE_ETHERNET cIPV4 ARP_HLEN_ETHERNET ARP_PLEN_IPV4 oper
//                                                 6B  4B  6B  4B
#define hARP4(sha, spa, tha, tpa)                  sha spa tha tpa
//                                                 3+1b    12b 2B
#define hVLAN(pcp_cfi, vid, ether_type)            pcp_cfi vid ether_type
#define hVXLAN()                                   "0000000000000000"
#define hBAAS()                                    "0000000000000000"
//           (3b+5x1b)  1        2
#define hGTP(vsn_flags, msgtype, msglen)           vsn_flags msgtype msglen
//             3        1
#define hGTPv2(snumber, reserved)                  snumber reserved
//              2        1          1
#define hGTPopt(seq_num, n_pdu_num, next_type)     seq_num n_pdu_num next_type
#define hRLC()                                     "00000000000000000000000000"
#define hPDCPbefore()                              "00000000000000000000000000000000000000000000000000000000000000000000"
#define hPDCP()                                    "0000000000"
#define PAYLOAD(payload)                           payload


// ------------------------------------------------------
// TODO

// TODO remove the following and replace with descriptions like this:

// ETH : hETH4(dsteth/6B, srceth/6B)
// ETH6: hETH(dsteth/6B, srceth/6B, cIPV6)
// ARP : hETH(dsteth/6B, srceth/6B, cARP)
// VLAN: hETH(dsteth/6B, srceth/6B, cVLAN)

// UDP  : ETH srcip4/4B hUDP(src_port/2B, dst_port/2B, len/2B, chksm/2B)
// GTP  : UDP hGTP(vsn_flags/3b+5x1b, msgtype/1B, msglen/2B)
// GTPv2: GTP hGTPv2(snumber/3B, reserved/1B)


// bytes: 6, 6
#define ETH(dsteth, srceth, ...)        FDATA(hETH4(dsteth, srceth), ##__VA_ARGS__)
#define ETH6(dsteth, srceth, ...)       FDATA(hETH6(dsteth, srceth), ##__VA_ARGS__)
#define ARP(dsteth, srceth, ...)        FDATA(hETH(dsteth, srceth, cARP), ##__VA_ARGS__)
#define VLAN(dsteth, srceth, ...)       FDATA(hETH(dsteth, srceth, cVLAN), ##__VA_ARGS__)

#define IPV4(dsteth, dstip4, srceth, srcip4, ...)              ETH( dsteth, srceth, hIPv4(srcip4, dstip4), ##__VA_ARGS__)
#define ICMP(dsteth, dstip4, srceth, srcip4, ...)              ETH( dsteth, srceth, hICMPv4(srcip4, dstip4), ##__VA_ARGS__)
#define IPV6(dsteth, dstip6, srceth, srcip6, next_hdr, ...)    ETH6(dsteth, srceth, hIP6(next_hdr, srcip6, dstip6), ##__VA_ARGS__)

#define UDP(dsteth, srceth, srcip4, src_port, dst_port, len, chksm, ...)   ETH( dsteth, srceth, srcip4, hUDP(src_port, dst_port, len, chksm), ##__VA_ARGS__)

// bytes: 6, 6, [6], 2, 6, 4, 6, 4, payload
#define ARP_IPV4(dsteth, srceth, arp_oper, sendereth, senderip4, targeteth, targetip4, ...)   ARP(dsteth, srceth, hARP(arp_oper), sendereth, senderip4, targeteth, targetip4, ##__VA_ARGS__)

#define VXLAN(dsteth, srceth, srcip4, ...)               UDP(dsteth, pVXLAN,  srceth, srcip4, PORT0, PORT0, LEN0, CHKSM0, ##__VA_ARGS__)
#define PHYS_BUFFER(dsteth, srceth, srcip4, ...)         UDP(dsteth, pBUF, srceth, srcip4, PORT0, PORT0, LEN0, CHKSM0, ##__VA_ARGS__)
#define RLC(dsteth, srceth, srcip4, ...)                 UDP(dsteth, pRLC,   srceth, srcip4, PORT0, PORT0, LEN0, CHKSM0, ##__VA_ARGS__)

// bytes:   6       6       4       2         2         2    2      (3b+5x1b)  1        2
#define GTP(dsteth, srceth, srcip4, src_port, dst_port, len, chksm, vsn_flags, msgtype, msglen, ...)   UDP(dsteth, pGTPU, srceth, srcip4, src_port, dst_port, len, chksm, hGTP(vsn_flags, msgtype, msglen), ##__VA_ARGS__)

#define GTPv1(dsteth, srceth, srcip4, vsn_flags, msgtype, msglen, ...) GTP(dsteth, srceth, srcip4, msgtype, msglen, ##__VA_ARGS__)
// bytes:     6       6       4       2         2         2    2      (3b+5x1b)  1        2       3        1
#define GTPv2(dsteth, srceth, srcip4, src_port, dst_port, len, chksm, vsn_flags, msgtype, msglen, snumber, reserved, ...) GTP(dsteth, srceth, srcip4, src_port, dst_port, len, chksm, vsn_flags, msgtype, msglen, hGTPv2(snumber, reserved), ##__VA_ARGS__)
