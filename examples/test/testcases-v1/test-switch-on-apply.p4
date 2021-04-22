
#include <core.p4>
#include <v1model.p4>
#include "../../include/std_headers.p4"

struct metadata {
}

struct headers {
    bits8_t h8;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.h8);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action nop()     { }
    action action1() { hdr.h8.f8 = 0x12; }
    action action2() { hdr.h8.f8 = 0x34; }

    table t1 {
        actions = {
            nop;
            action1;
        }

        key = {
            hdr.h8.f8: exact;
        }

        size = 1;

        default_action = nop;

        const entries = {
            0xAA : action1;
        }
    }

    table t2 {
        actions = {
            nop;
            action2;
        }

        key = {
            hdr.h8.f8: exact;
        }

        size = 1;

        default_action = nop;

        const entries = {
            0x12 : action2;
        }
    }

    apply {
        switch (t1.apply().action_run) {
            action1: {
                t2.apply();
            }
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.h8);
    }
}

#include "v1-testcase-dummy-pipeline.p4"
