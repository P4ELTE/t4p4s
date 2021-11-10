/* -*- P4_16 -*- */

#include <core.p4>
#include <v1model.p4>

header ethernet_header_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct parsed_packet {
    ethernet_header_t ethernet;
}

struct metadata {
}

parser MyParser(packet_in pkt11111, out parsed_packet hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        pkt11111.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0x0800: accept;
            default: accept;
        }
    }
}

control MyDeparser(packet_out pkt2222, in parsed_packet hdr) {
    apply { pkt2222.emit(hdr.ethernet); }
}

register<bit<32>>(1) test_register;

control MyIngress(inout parsed_packet hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action _drop() {
        mark_to_drop(standard_metadata);
    }

    action forward(bit<9> port) {
        standard_metadata.egress_port = port;
    }

    action mac_learn() {
    }

    action _nop() {
    }

    action bcast() {
		standard_metadata.egress_port = 100;
    }

    table smac {
        key = {
            standard_metadata.ingress_port: exact;
        }
        actions = {
            forward;
            mac_learn;
            _drop;
            _nop;
        }
        default_action = _drop;
    }

    table dmac {
        key = {
            standard_metadata.ingress_port: exact;
        }
        actions = {
            bcast;
            forward;
        }
        default_action = bcast;
    }

    apply {
        bit<32> test_val;
        test_register.read(test_val, 0);
        smac.apply();
        dmac.apply();
    }
}

control MyEgress(inout parsed_packet hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply { }
}

control MyVerify(inout parsed_packet hdr, inout metadata meta) {
    apply { }
}

control MyCalculate(inout parsed_packet hdr, inout metadata meta) {
    apply { }
}

V1Switch(
    MyParser(),
    MyVerify(),
    MyIngress(),
    MyEgress(),
    MyCalculate(),
    MyDeparser()
) main;

