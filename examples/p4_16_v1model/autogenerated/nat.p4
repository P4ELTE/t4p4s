#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<16> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
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
    bit<16> len;
    bit<16> checksum;
}

struct metadata {
}

struct headers {
    @name("ethernet") 
    ethernet_t ethernet;
    @name("ipv4") 
    ipv4_t     ipv4;
    @name("tcp") 
    tcp_t      tcp;
    @name("udp") 
    udp_t      udp;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    @name(".parse_ipv4") state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            8w0x6: parse_tcp;
            8w0x11: parse_udp;
            default: accept;
        }
    }
    @name(".parse_tcp") state parse_tcp {
        packet.extract(hdr.tcp);
        transition accept;
    }
    @name(".parse_udp") state parse_udp {
        packet.extract(hdr.udp);
        transition accept;
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

@name("natTcp_learn_digest") struct natTcp_learn_digest {
    bit<32> srcAddr;
    bit<16> srcPort;
}

@name("natUdp_learn_digest") struct natUdp_learn_digest {
    bit<32> srcAddr;
    bit<16> srcPort;
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".resolve_dstTcp") action resolve_dstTcp(bit<32> ipAddr, bit<16> tcpPort) {
        hdr.ipv4.dstAddr = ipAddr;
        hdr.tcp.dstPort = tcpPort;
    }
    @name("._drop") action _drop() {
        mark_to_drop();
    }
    @name(".resolve_dstUdp") action resolve_dstUdp(bit<32> ipAddr, bit<16> udpPort) {
        hdr.ipv4.dstAddr = ipAddr;
        hdr.udp.dstPort = udpPort;
    }
    @name(".natTcp_learn") action natTcp_learn() {
        digest<natTcp_learn_digest>((bit<32>)1025, { hdr.ipv4.srcAddr, hdr.tcp.srcPort });
    }
    @name(".encodeTcp_src") action encodeTcp_src(bit<32> ipAddr, bit<16> tcpPort) {
        hdr.ipv4.srcAddr = ipAddr;
        hdr.tcp.srcPort = tcpPort;
    }
    @name(".natUdp_learn") action natUdp_learn() {
        digest<natUdp_learn_digest>((bit<32>)1026, { hdr.ipv4.srcAddr, hdr.udp.srcPort });
    }
    @name(".encodeUdp_src") action encodeUdp_src(bit<32> ipAddr, bit<16> udpPort) {
        hdr.ipv4.srcAddr = ipAddr;
        hdr.udp.srcPort = udpPort;
    }
    @name("._no_op") action _no_op() {
        ;
    }
    @name(".nat_dl_tcp") table nat_dl_tcp {
        actions = {
            resolve_dstTcp;
            _drop;
        }
        key = {
            hdr.ipv4.dstAddr: exact;
            hdr.tcp.dstPort : exact;
        }
    }
    @name(".nat_dl_udp") table nat_dl_udp {
        actions = {
            resolve_dstUdp;
            _drop;
        }
        key = {
            hdr.ipv4.dstAddr: exact;
            hdr.udp.dstPort : exact;
        }
    }
    @name(".nat_ul_tcp") table nat_ul_tcp {
        actions = {
            natTcp_learn;
            encodeTcp_src;
        }
        key = {
            hdr.ipv4.srcAddr: exact;
            hdr.tcp.srcPort : exact;
        }
    }
    @name(".nat_ul_udp") table nat_ul_udp {
        actions = {
            natUdp_learn;
            encodeUdp_src;
        }
        key = {
            hdr.ipv4.srcAddr: exact;
            hdr.udp.srcPort : exact;
        }
    }
    @name(".own_pool") table own_pool {
        actions = {
            _no_op;
        }
        key = {
            hdr.ipv4.dstAddr: exact;
        }
    }
    apply {
        if (own_pool.apply().hit) {
            if (hdr.tcp.isValid()) {
                nat_dl_tcp.apply();
            }
            if (hdr.udp.isValid()) {
                nat_dl_udp.apply();
            }
        }
        else {
            if (hdr.tcp.isValid()) {
                nat_ul_tcp.apply();
            }
            if (hdr.udp.isValid()) {
                nat_ul_udp.apply();
            }
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.udp);
        packet.emit(hdr.tcp);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta) {
    Checksum16() ipv4_checksum;
    Checksum16() tcp_checksum;
    apply {
        if (hdr.ipv4.isValid() && hdr.ipv4.hdrChecksum == ipv4_checksum.get({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr })) 
            mark_to_drop();
        if (hdr.tcp.isValid() && hdr.tcp.checksum == tcp_checksum.get({ hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, 8w0, hdr.ipv4.protocol, hdr.ipv4.totalLen, 16w0xffeb, hdr.tcp.srcPort, hdr.tcp.dstPort, hdr.tcp.seqNo, hdr.tcp.ackNo, hdr.tcp.dataOffset, hdr.tcp.res, hdr.tcp.flags, hdr.tcp.window, hdr.tcp.urgentPtr })) 
            mark_to_drop();
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    Checksum16() ipv4_checksum;
    Checksum16() tcp_checksum;
    apply {
        if (hdr.ipv4.isValid()) 
            hdr.ipv4.hdrChecksum = ipv4_checksum.get({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr });
        if (hdr.tcp.isValid()) 
            hdr.tcp.checksum = tcp_checksum.get({ hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, 8w0, hdr.ipv4.protocol, hdr.ipv4.totalLen, 16w0xffeb, hdr.tcp.srcPort, hdr.tcp.dstPort, hdr.tcp.seqNo, hdr.tcp.ackNo, hdr.tcp.dataOffset, hdr.tcp.res, hdr.tcp.flags, hdr.tcp.window, hdr.tcp.urgentPtr });
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
