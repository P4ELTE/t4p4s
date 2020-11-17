
#include "../include/std_headers.p4"

struct metadata {
}

struct headers {
    ethernet_t ethernet;
    hdr1_t     h1;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.ethernet);
        packet.extract(hdr.h1);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action nop() {}
    action action1() { hdr.h1.byte1 = hdr.h1.byte1 | 1; }
    action action2() {}
    action action3(bit<9> x) { standard_metadata.egress_port = x; }

    table t1 {
        actions = {
            nop;
            action1;
            action2;
        }

        key = {
            hdr.ethernet.srcAddr: ternary;
        }

        size = 1;

        default_action = nop;

        const entries = {
            123 : action1;
            _   : action2;
        }
    }

    table t2 {
        actions = {
            nop;
            action1;
            action2;
            action3;
        }

        key = {
            hdr.ethernet.dstAddr: exact;
            hdr.ethernet.srcAddr: exact;
        }

        size = 1;

        default_action = nop;

        const entries = {
            (0x12345678, 0x9ABCDEF0) : action1;
            (100, 200)               : action2;
            (300, 400)               : action3(500);
            (_, _)                   : action3(123);

            #ifdef T4P4S_TEST_1
                    (0xD15EA5E, 0XDEAD_BEEF): action1;
            #else
                    (0xDEAD_10CC, 0xBAAAAAAD): action2;
            #endif
        }
    }

    apply {
        t1.apply();
        t2.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.h1);
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) { apply {} }
control verifyChecksum(inout headers hdr, inout metadata meta) { apply {} }
control computeChecksum(inout headers hdr, inout metadata meta) { apply {} }

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
