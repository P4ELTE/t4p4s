
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


header bits1_t   { bit<1>  f1;  }
header bits2_t   { bit<2>  f2;  }
header bits3_t   { bit<3>  f3;  }
header bits4_t   { bit<4>  f4;  }
header bits5_t   { bit<5>  f5;  }
header bits6_t   { bit<6>  f6;  }
header bits7_t   { bit<7>  f7;  }
header bits8_t   { bit<8>  f8;  }
header bits9_t   { bit<9>  f9;  }
header bits10_t  { bit<10> f10;  }
header bits11_t  { bit<11> f11;  }
header bits12_t  { bit<12> f12;  }
header bits13_t  { bit<13> f13;  }
header bits14_t  { bit<14> f14;  }
header bits15_t  { bit<15> f15;  }
header bits16_t  { bit<16> f16;  }
header bits17_t  { bit<17> f17;  }
header bits18_t  { bit<18> f18;  }
header bits19_t  { bit<19> f19;  }
header bits20_t  { bit<20> f20;  }
header bits21_t  { bit<21> f21;  }
header bits22_t  { bit<22> f22;  }
header bits23_t  { bit<23> f23;  }
header bits24_t  { bit<24> f24;  }
header bits25_t  { bit<25> f25;  }
header bits26_t  { bit<26> f26;  }
header bits27_t  { bit<27> f27;  }
header bits28_t  { bit<28> f28;  }
header bits29_t  { bit<29> f29;  }
header bits30_t  { bit<30> f30;  }
header bits31_t  { bit<31> f31;  }
header bits32_t  { bit<32> f32;  }
