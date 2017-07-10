#include <core.p4>
#include <v1model.p4>

struct routing_metadata_t {
    bit<32> nhgroup;
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
    @name("routing_metadata") 
    routing_metadata_t routing_metadata;
}

struct headers {
    @name("ethernet") 
    ethernet_t ethernet;
    @name("ipv4") 
    ipv4_t     ipv4;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    @name("parse_ipv4") state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition accept;
    }
    @name("start") state start {
        transition parse_ethernet;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("r1") register<bit<32>>(32w2) r1;
    @name("set_nhop") action set_nhop(bit<32> nhgroup) {
        meta.routing_metadata.nhgroup = nhgroup;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
        r1.write((bit<32>)0, (bit<32>)40);
        r1.read(hdr.ipv4.ttl, (bit<32>)0);
    }
    @name("_drop") action _drop() {
        mark_to_drop();
    }
    @name("forward") action forward(bit<48> dmac, bit<48> smac, bit<9> port) {
        hdr.ethernet.dstAddr = dmac;
        standard_metadata.egress_port = port;
        hdr.ethernet.srcAddr = smac;
    }
    @name("ipv4_lpm") table ipv4_lpm() {
        actions = {
            set_nhop;
            _drop;
            NoAction;
        }
        key = {
            hdr.ipv4.dstAddr: lpm;
        }
        size = 1024;
        default_action = NoAction();
    }
    @name("nexthops") table nexthops() {
        actions = {
            forward;
            _drop;
            NoAction;
        }
        key = {
            meta.routing_metadata.nhgroup: exact;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        ipv4_lpm.apply();
        nexthops.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta) {
    Checksum16() ipv4_checksum;
    apply {
        if (hdr.ipv4.isValid() && hdr.ipv4.hdrChecksum == ipv4_checksum.get({ hdr.ipv4.versionIhl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr })) 
            mark_to_drop();
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    Checksum16() ipv4_checksum;
    apply {
        if (hdr.ipv4.isValid()) 
            hdr.ipv4.hdrChecksum = ipv4_checksum.get({ hdr.ipv4.versionIhl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr });
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
