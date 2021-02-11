
#include "../include/std_headers.p4"

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
    
    table t1 {
        actions = {
            nop;
            action1;
        }

        key = {
            hdr.ethernet.srcAddr: ternary;
        }

        size = 1;

        default_action = nop;

        const entries = {
            0x112233445566 &&& 0xF0000000000F : action1;
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
