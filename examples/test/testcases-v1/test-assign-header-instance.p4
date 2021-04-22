// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Eotvos Lorand University, Budapest, Hungary

#include <core.p4>
#include <v1model.p4>
#include "../../include/std_headers.p4"

struct metadata {
}

struct headers {
    ethernet_t  ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action set_ethertype() {
        hdr.ethernet.etherType = 0x1234;
        hdr.ethernet.dstAddr   = 0b0001_0010_0011_0100_0101_0110_0111_1000_1001_1010_1011_1100;

        standard_metadata.egress_port = 0;
    }

    table dmac {
        actions = {
            set_ethertype;
        }

        key = {
        }

        size = 1;

        default_action = set_ethertype;
    }

    apply {
        dmac.apply();
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
