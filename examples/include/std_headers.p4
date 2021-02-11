#include <core.p4>
#include <v1model.p4>

header cpu_header_t {
    bit<64> preamble;
    bit<8>  device;
    bit<8>  reason;
    bit<8>  if_index;
}

header ethernet_t {
    bit<48>   dstAddr;
    bit<48>   srcAddr;
    bit<16>   etherType;
}

header ipv4_t {
    bit<4>    version;
    bit<4>    ihl;
    bit<8>    diffserv;
    bit<16>   totalLen;
    bit<16>   identification;
    bit<3>    flags;
    bit<13>   fragOffset;
    bit<8>    ttl;
    bit<8>    protocol;
    bit<16>   hdrChecksum;
    bit<32>   srcAddr;
    bit<32>   dstAddr;
}

header vxlan_t  {
    bit<8> flags;
    bit<24> reserved1;
    bit<24> vni;
    bit<8> reserved2;
    
}

header icmp_t {
    bit<8>  type;
    bit<8>  code;
    bit<16> checksum;
}

header gtp_t {
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

header tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<4>  res;
    bit<8>  flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

header udp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<16> plength;
    bit<16> checksum;
}

header arp_t {
    bit<16> htype;
    bit<16> ptype;
    bit<8>  hlen;
    bit<8>  plen;
    bit<16> oper;
}

header arp_ipv4_t {
    bit<48>  sha;
    bit<32> spa;
    bit<48>  tha;
    bit<32> tpa;
}




header hdr1_t {
    bit<8> byte1;
}

header test_dstAddr_t {
    bit<48> dstAddr;
}

header test_srcAddr_t {
    bit<48> srcAddr;
}

header test_etherType_t {
    bit<16> etherType;
}

