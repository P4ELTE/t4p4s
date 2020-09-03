/*******************************************************************************
 * BAREFOOT NETWORKS CONFIDENTIAL & PROPRIETARY
 *
 * Copyright (c) 2019-present Barefoot Networks, Inc.
 *
 * All Rights Reserved.
 *
 * NOTICE: All information contained herein is, and remains the property of
 * Barefoot Networks, Inc. and its suppliers, if any. The intellectual and
 * technical concepts contained herein are proprietary to Barefoot Networks, Inc.
 * and its suppliers and may be covered by U.S. and Foreign Patents, patents in
 * process, and are protected by trade secret or copyright law.  Dissemination of
 * this information or reproduction of this material is strictly forbidden unless
 * prior written permission is obtained from Barefoot Networks, Inc.
 *
 * No warranty, explicit or implicit is provided, unless granted under a written
 * agreement with Barefoot Networks, Inc.
 *
 ******************************************************************************/

#ifndef _HEADERS_
#define _HEADERS_

///
/// CONSTANTS
///

typedef bit<48> mac_addr_t;
typedef bit<32> ipv4_addr_t;
typedef bit<128> ipv6_addr_t;
typedef bit<12> vlan_id_t;

#ifdef __TARGET_PSA__
#define CounterType_t PSA_CounterType_t
#endif

#ifdef __TARGET_V1__
typedef bit<9> PortId_t;
#endif

typedef bit<16> ether_type_t;
const ether_type_t ETHERTYPE_IPV4 = 16w0x0800;
const ether_type_t ETHERTYPE_ARP  = 16w0x0806;
const ether_type_t ETHERTYPE_IPV6 = 16w0x86dd;
const ether_type_t ETHERTYPE_VLAN = 16w0x8100;

typedef bit<8> ip_proto_t;
const ip_proto_t IPPROTO_ICMP = 1;
const ip_proto_t IPPROTO_IP   = 4;
const ip_proto_t IPPROTO_TCP  = 6;
const ip_proto_t IPPROTO_UDP  = 17;

const bit<16> ARP_HTYPE_ETHERNET = 0x0001;
const bit<16> ARP_PTYPE_IPV4     = 0x0800;
const bit<8>  ARP_HLEN_ETHERNET  = 6;
const bit<8>  ARP_PLEN_IPV4      = 4;
const bit<16> ARP_OPER_REQUEST   = 1;
const bit<16> ARP_OPER_REPLY     = 2;

const bit<8> ICMP_ECHO_REQUEST   = 8;
const bit<8> ICMP_ECHO_REPLY     = 0;

const bit<16> UDP_PORT_GTPC      = 2123;
const bit<16> UDP_PORT_GTPU      = 2152;
const bit<16> UDP_PORT_VXLAN     = 4789;

//const bit<48> EPG_VIRT_MAC       = 48w0x11_22_33_44_55_66;	// FIXME, set it configurable
#define EPG_VIRT_MAC 48w0x112233445566		// FIXME, set it configurable
const bit<32> EPG_IP             = 0x0A000001; 			// FIXME, set configurable (now 10.0.0.1)
const bit<3>  MAC_LEARN_DIGEST   = 1;
const bit<3>  ARP_MISS_DIGEST    = 2;

///
/// HEADERS
///

header ethernet_h {
    mac_addr_t dst_addr;
    mac_addr_t src_addr;
    bit<16> ether_type;
}

header vlan_tag_h {
    bit<3> pcp;
    bit<1> cfi;
    vlan_id_t vid;
    bit<16> ether_type;
}

header mpls_h {
    bit<20> label;
    bit<3> exp;
    bit<1> bos;
    bit<8> ttl;
}

header ipv4_h {
    bit<4> version;
    bit<4> ihl;
    bit<8> diffserv;
    bit<16> total_len;
    bit<16> identification;
    bit<3> flags;
    bit<13> frag_offset;
    bit<8> ttl;
    bit<8> protocol;
    bit<16> hdr_checksum;
    ipv4_addr_t src_addr;
    ipv4_addr_t dst_addr;
}

header ipv6_h {
    bit<4> version;
    bit<8> traffic_class;
    bit<20> flow_label;
    bit<16> payload_len;
    bit<8> next_hdr;
    bit<8> hop_limit;
    ipv6_addr_t src_addr;
    ipv6_addr_t dst_addr;
}

header tcp_h {
    bit<16> src_port;
    bit<16> dst_port;
    bit<32> seq_no;
    bit<32> ack_no;
    bit<4> data_offset;
    bit<4> res;
    bit<8> flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgent_ptr;
}

header udp_h {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> length;
    bit<16> checksum;
}

header icmp_h {
    bit<8> type;
    bit<8> code;
    bit<16> hdr_checksum;
}

// Address Resolution Protocol -- RFC 6747
header arp_generic_h {
    bit<16> htype;
    bit<16> ptype;
    bit<8> hlen;
    bit<8> plen;
    bit<16> oper;
}

header arp_ipv4_h {
    bit<48> sha;
    bit<32> spa;
    bit<48> tha;
    bit<32> tpa;
}

// Segment Routing Extension (SRH) -- IETFv7
header ipv6_srh_h {
    bit<8> next_hdr;
    bit<8> hdr_ext_len;
    bit<8> routing_type;
    bit<8> seg_left;
    bit<8> last_entry;
    bit<8> flags;
    bit<16> tag;
}

// VXLAN -- RFC 7348
header vxlan_h {
    bit<8> flags;
    bit<24> reserved;
    bit<24> vni;
    bit<8> reserved2;
}

// Generic Routing Encapsulation (GRE) -- RFC 1701
header gre_h {
    bit<1> C;
    bit<1> R;
    bit<1> K;
    bit<1> S;
    bit<1> s;
    bit<3> recurse;
    bit<5> flags;
    bit<3> version;
    bit<16> proto;
}

// GTP-U header -- 3GPP
header gtp_h {
    bit<3> version; /* this should be 1 for GTPv1 and 2 for GTPv2 */
    bit<1> pFlag;   /* protocolType for GTPv1 and pFlag for GTPv2 */
    bit<1> reserved;
    bit<1> eFlag;   /* only used by GTPv1 - E flag */
    bit<1> sFlag;   /* only used by GTPv1 - S flag */
    bit<1> pnFlag;  /* only used by GTPv1 - PN flag */
    bit<8> messageType;
    bit<16> messageLength;
    bit<32> teid;
}

/*struct header_t {
    ethernet_h ethernet;
    vlan_tag_h vlan_tag;
    ipv4_h ipv4;
    ipv6_h ipv6;
    tcp_h tcp;
    udp_h udp;

    // Add more headers here.
}*/

/*struct header_t {
    ethernet_h[3] ethernet;	// [0]: outer L2, [1]: MAC-in-MAC or VxLAN, [2]: VxLAN in MAC-in-MAC
    ipv4_h[3]     ipv4;		// [0]: outer L3, [1]: GRE, VxLAN, IPIP or GTP, [2]: GTP in VxLAN/GRE
    icmp_h        icmp;
    udp_h[2]      udp;
    tcp_h[2]      tcp;
    vxlan_h       vxlan;
    vxlan_h       vxlan_new;
    arp_generic_h arp;
    arp_ipv4_h    arp_ipv4;
    gtp_h 	  gtp;
}*/

header digest_h {
    bit<48>       mac_addr;
}

struct header_t {
    ethernet_h    eth;
    vlan_tag_h    vlan;
    arp_generic_h arp;
    arp_ipv4_h    arp_ipv4;
    icmp_h        icmp;
    ipv4_h	  ipv4;
    ipv6_h	  ipv6;
    udp_h         udp;
    vxlan_h       vxlan;
    ethernet_h    eth_x;
    ipv4_h	  ipv4_x;
    ipv6_h	  ipv6_x;
    udp_h         udp_x;
    gtp_h 	  gtp;
    ipv4_h	  ipv4_u;
    udp_h         udp_u;
    digest_h      digest_hack;
}

struct empty_header_t {}

struct empty_metadata_t {}

struct EMPTY {}

#endif /* _HEADERS_ */
