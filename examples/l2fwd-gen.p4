#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata {
}

struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
    ethernet_t ethernet2;
    ethernet_t ethernet3;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        packet.extract(hdr.ethernet2);
        packet.extract(hdr.ethernet3);
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

@name("mac_learn_digest") struct mac_learn_digest {
    bit<48> srcAddr;
    bit<9>  ingress_port;
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".forward") action forward(bit<9> port) {
        standard_metadata.egress_port = port;
    }
    @name(".bcast") action bcast() {
        standard_metadata.egress_port = 9w100;
    }
    @name(".mac_learn") action mac_learn() {
        digest<mac_learn_digest>((bit<32>)1024, { hdr.ethernet.srcAddr, standard_metadata.ingress_port });
    }
    @name("._nop") action _nop() {
        hdr.ethernet2.setInvalid();
    }
    @name("._nop") action testing(bit<32> arg1, bit<32> arg2) {
    }
    @name(".dmac") table dmac {
        actions = {
            forward;
            bcast;
            testing;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        size = 512;

        #ifdef T4P4S_TEST_1
            const entries = {
                (0xD15EA5E): bcast();
            }
        #elif T4P4S_TEST_abc
            const entries = {
                (0xDEAD_10CC): testing(510, 0xffff_ffff);
            }
        #endif
    }
    @name(".smac") table smac {
        actions = {
            mac_learn;
            _nop;
        }
        key = {
            hdr.ethernet.srcAddr: exact;
        }
        size = 512;
    }
    apply {
        smac.apply();
        dmac.apply();
    }

}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
//        packet.emit(hdr.ethernet2);
        packet.emit(hdr.ethernet3);
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

