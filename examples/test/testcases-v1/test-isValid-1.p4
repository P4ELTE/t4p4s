// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include <core.p4>
#include <v1model.p4>
#include "../../include/std_headers.p4"

struct metadata {
}

struct headers {
    test_dstAddr_t   dstAddr;
    test_srcAddr_t   srcAddr;
    test_etherType_t etherType;
    test_srcAddr_t   srcAddr2;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.dstAddr);
        packet.extract(hdr.srcAddr);
        packet.extract(hdr.etherType);
        packet.extract(hdr.srcAddr2);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action setInvalid_srcAddr2() {
        hdr.srcAddr2.setInvalid();
    }

    table dmac {
        actions = {
            setInvalid_srcAddr2;
        }

        key = {
        }

        size = 1;

        default_action = setInvalid_srcAddr2;
    }

    apply {
        dmac.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        if (hdr.dstAddr.isValid()) {
            packet.emit(hdr.dstAddr);
        }
        if (hdr.srcAddr.isValid()) {
            packet.emit(hdr.srcAddr);
        }
        if (hdr.srcAddr2.isValid()) {
            packet.emit(hdr.srcAddr2);
        }
        if (hdr.etherType.isValid()) {
            packet.emit(hdr.etherType);
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) { apply {} }
control verifyChecksum(inout headers hdr, inout metadata meta) { apply {} }
control computeChecksum(inout headers hdr, inout metadata meta) { apply {} }

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
