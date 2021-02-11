#include <core.p4>
#include <v1model.p4>

struct routing_metadata_t {
    bit<32> nhgroup;
}

header arp_t {
    bit<16> hardware_type;
    bit<16> protocol_type;
    bit<8>  HLEN;
    bit<8>  PLEN;
    bit<16> OPER;
    bit<48> sender_ha;
    bit<32> sender_ip;
    bit<48> target_ha;
    bit<32> target_ip;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<8>  versionIhl;
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

struct metadata {
    @name(".routing_metadata") 
    routing_metadata_t routing_metadata;
}

struct headers {
    @name(".arp") 
    arp_t      arp;
    @name(".ethernet") 
    ethernet_t ethernet;
    @name(".ipv4") 
    ipv4_t     ipv4;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_arp") state parse_arp {
        packet.extract(hdr.arp);
        transition accept;
    }
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            16w0x806: parse_arp;
            default: accept;
        }
    }
    @name(".parse_ipv4") state parse_ipv4 {
        packet.extract(hdr.ipv4);
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

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set_nhop") action set_nhop(bit<32> nhgroup) {
        meta.routing_metadata.nhgroup = nhgroup;
    }
    @name("._drop") action _drop() {
        mark_to_drop(standard_metadata);
    }
    @name("._nop") action _nop() {
    }
    @name(".forward") action forward(bit<48> dmac_val, bit<48> smac_val, bit<9> port) {
        hdr.ethernet.dstAddr = dmac_val;
        standard_metadata.egress_port = port;
        hdr.ethernet.srcAddr = smac_val;
        hdr.ipv4.ttl = hdr.ipv4.ttl - 8w1;
    }
    @name(".ipv4_lpm") table ipv4_lpm {
        actions = {
            set_nhop;
            _drop;
        }
        key = {
            hdr.ipv4.dstAddr: lpm;
        }
        size = 1024;
    }
    @name(".macfwd") table macfwd {
        actions = {
            _nop;
            _drop;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        size = 256;
    }
    @name(".nexthops") table nexthops {
        actions = {
            forward;
            _drop;
        }
        key = {
            meta.routing_metadata.nhgroup: exact;
        }
        size = 512;
    }
    apply {
        if (macfwd.apply().hit) {
            ipv4_lpm.apply();
            nexthops.apply();
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.arp);
        packet.emit(hdr.ipv4);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

