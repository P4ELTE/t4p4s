// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include <core.p4>
#include <v1model.p4>
#include "../include/std_headers.p4"


register<bit<32>>(32w100) debug;
register<bit<1>>(32w1)    reg1;


struct gtp_metadata_t {
    bit<1>  data1;
    bit<16> data16;
    bit<32> data32;

    bit<1>  regtmp1;
    bit<16> regtmp16;
    bit<32> regtmp32;
}

struct metadata {
    gtp_metadata_t gtp_metadata;
}

struct headers {
    test_dstAddr_t   dstAddr;
    test_srcAddr_t   srcAddr;
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
    counter(  1, CounterType.packets)             c_p;
    counter( 10, CounterType.bytes)               c_b;
    counter(100, CounterType.packets_and_bytes)   c_pb;

    direct_counter(CounterType.packets)           dc_p;
    direct_counter(CounterType.bytes)             dc_b;
    direct_counter(CounterType.packets_and_bytes) dc_pb;

    meter(1, MeterType.bytes)     m_b;
    meter(256, MeterType.packets) m_p;

    direct_meter<bit<1>>(MeterType.bytes)    dm_b;
    direct_meter<bit<32>>(MeterType.packets) dm_p;

    register<bit<16>>(32w65536) reg16;


    action forward() {}
    action _nop() {}

    action bcast() {
        c_p.count(1);

        c_b.count(2);
        c_b.count(2);

        c_pb.count(3);
        c_pb.count(3);
        c_pb.count(3);

        // m_b.execute_meter<bit<32>>(1, meta.gtp_metadata.data32);
        // m_p.execute_meter<bit<32>>(11, meta.gtp_metadata.data32);


        debug.write(99, 12345678);



        // dm_b.read(meta.gtp_metadata.data1);
        // dm_p.read(meta.gtp_metadata.data32);

        // debug.read(meta.gtp_metadata.regtmp32, 99);
        // debug.write(99, 12345678);
        // debug.read(meta.gtp_metadata.regtmp32, 99);

        // reg1.read(meta.gtp_metadata.regtmp1, 1);
        // reg1.write(1, 1);
        // reg1.read(meta.gtp_metadata.regtmp1, 1);

        // reg16.read(meta.gtp_metadata.regtmp16, 65535);
        // reg16.write(65535, 0xf0f0);
        // reg16.read(meta.gtp_metadata.regtmp16, 65535);
    }

    action mac_learn() {
        bcast();
    }


    table smac {
        key = {
        }

        actions = {
            mac_learn;
            _nop;
        }
    }

    table dmac {
        key = {
        }

        actions = {
            bcast;
            forward;
        }

        size = 1;

        counters = { c_p, c_b, c_pb, dc_p, dc_b, dc_pb };
        meters   = { m_b, m_p, dm_b, dm_p };
    }

    apply {
        dmac.apply();
        smac.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.dstAddr);
        packet.emit(hdr.srcAddr);
        packet.emit(hdr.etherType);
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) { apply {} }
control verifyChecksum(inout headers hdr, inout metadata meta) { apply {} }
control computeChecksum(inout headers hdr, inout metadata meta) { apply {} }

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
