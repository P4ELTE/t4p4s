// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include <core.p4>
#include <v1model.p4>
#include "../include/std_headers.p4"

struct metadata {
}

struct headers {
    test_dstAddr_t   dstAddr;
    test_srcAddr_t   srcAddrIn;
    test_srcAddr_t   srcAddrAdded;
    test_etherType_t etherType;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.dstAddr);
        packet.extract(hdr.srcAddrIn);
        packet.extract(hdr.etherType);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action setValid_srcAddrAdded() {
        hdr.srcAddrAdded.setValid();
        hdr.srcAddrAdded.srcAddr = hdr.srcAddrIn.srcAddr;
    }

    table dmac {
        actions = {
            setValid_srcAddrAdded;
        }

        key = {
        }

        size = 1;

        default_action = setValid_srcAddrAdded;
    }

    apply {
        dmac.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.dstAddr);
        packet.emit(hdr.srcAddrIn);
        packet.emit(hdr.srcAddrAdded);
        packet.emit(hdr.etherType);
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) { apply {} }
control verifyChecksum(inout headers hdr, inout metadata meta) { apply {} }
control computeChecksum(inout headers hdr, inout metadata meta) { apply {} }

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
