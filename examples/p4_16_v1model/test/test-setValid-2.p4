#include "common_headers_v1model.p4"

struct metadata {
}

struct headers {
    test_dstAddr_t   dstAddr;
    test_srcAddr_t   srcAddr;
    test_srcAddr_t   srcAddr2;
    test_etherType_t etherType;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.dstAddr);
        packet.extract(hdr.srcAddr);
        packet.extract(hdr.etherType);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action setValid_srcAddr2() {
        hdr.srcAddr2.setValid();
        hdr.srcAddr2.srcAddr = hdr.srcAddr.srcAddr;
    }

    table dmac {
        actions = {
            setValid_srcAddr2;
        }

        key = {
        }

        size = 1;
    }

    apply {
        dmac.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.dstAddr);
        packet.emit(hdr.srcAddr);
        packet.emit(hdr.srcAddr2);
        packet.emit(hdr.etherType);
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) { apply {} }
control verifyChecksum(inout headers hdr, inout metadata meta) { apply {} }
control computeChecksum(inout headers hdr, inout metadata meta) { apply {} }

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
