
#include <core.p4>
#include <v1model.p4>
#include "../../include/std_headers.p4"

struct metadata {
}

struct headers {
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action nop() {}
    action action1() { hdr.ethernet.srcAddr =  0x111111111111; }
    action action2() { hdr.ethernet.srcAddr =  0x999999999999; }
    action action3() { hdr.ethernet.srcAddr =  0x555555555555; }
    
    table t1 {
        actions = {
            nop;
            action1;
            action2;
            action3;
        }

        key = {
            hdr.ethernet.srcAddr: lpm;
        }

        size = 1;

        default_action = nop;

        const entries = {
            0x112233445500 &&& 0xFFF : action1;
            0x113300000000 &&& 0xFFF : action2;
            0x113300000000 &&& 0xF   : action3;
        }
    }

    apply {
        t1.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) { apply {} }
control verifyChecksum(inout headers hdr, inout metadata meta) { apply {} }
control computeChecksum(inout headers hdr, inout metadata meta) { apply {} }

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
