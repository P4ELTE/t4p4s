#include <core.p4>
#include <v1model.p4>

struct intrinsic_metadata_t {
    bit<4>  mcast_grp;
    bit<4>  egress_rid;
    bit<16> mcast_hash;
    bit<32> lf_field_list;
    bit<8>  fwd_mode;
}

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
    @name("intrinsic_metadata") 
    intrinsic_metadata_t intrinsic_metadata;
    @name("routing_metadata") 
    routing_metadata_t   routing_metadata;
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
    @name("rewrite_mac") action rewrite_mac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    @name("_nop") action _nop() {
    }
    @name("send_frame") table send_frame() {
        actions = {
            rewrite_mac;
            _nop;
            NoAction;
        }
        key = {
            standard_metadata.egress_port: exact;
        }
        default_action = NoAction();
    }
    apply {
        if (meta.intrinsic_metadata.fwd_mode != 8w0) {
            send_frame.apply();
        }
    }
}

@name("mac_learn_digest") struct mac_learn_digest {
    bit<48> srcAddr;
    bit<9>  ingress_port;
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("forward") action forward(bit<9> port) {
        standard_metadata.egress_port = port;
    }
    @name("broadcast") action broadcast() {
        standard_metadata.egress_port = 9w100;
    }
    @name("set_fwd_mode") action set_fwd_mode(bit<8> mode) {
        meta.intrinsic_metadata.fwd_mode = mode;
    }
    @name("l3forward") action l3forward(bit<48> dmac) {
        hdr.ethernet.dstAddr = dmac;
    }
    @name("_drop") action _drop() {
        mark_to_drop();
    }
    @name("set_nhop") action set_nhop(bit<32> nhgroup) {
        meta.routing_metadata.nhgroup = nhgroup;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name("mac_learn") action mac_learn() {
        digest<mac_learn_digest>((bit<32>)1024, { hdr.ethernet.srcAddr, standard_metadata.ingress_port });
    }
    @name("_nop") action _nop() {
    }
    @name("dmac") table dmac() {
        actions = {
            forward;
            broadcast;
            NoAction;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        size = 512;
        default_action = NoAction();
    }
    @name("dmac_classifier") table dmac_classifier() {
        actions = {
            set_fwd_mode;
            NoAction;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        default_action = NoAction();
    }
    @name("ipv4_forward") table ipv4_forward() {
        actions = {
            l3forward;
            _drop;
            NoAction;
        }
        key = {
            meta.routing_metadata.nhgroup: exact;
        }
        size = 512;
        default_action = NoAction();
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
    @name("smac") table smac() {
        actions = {
            mac_learn;
            _nop;
            NoAction;
        }
        key = {
            hdr.ethernet.srcAddr: exact;
        }
        size = 512;
        default_action = NoAction();
    }
    apply {
        smac.apply();
        if (dmac_classifier.apply().hit) 
            ;
        else {
            ipv4_lpm.apply();
            ipv4_forward.apply();
        }
        dmac.apply();
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
