// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "psa-boilerplate-pre.p4"

Register<bit<32>, int>(32w100) debug;
Register<bit<1>, int>(32w1)    reg1;


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

PARSER {
    state start {
        packet.extract(hdr.dstAddr);
        packet.extract(hdr.srcAddr);
        packet.extract(hdr.etherType);
        transition accept;
    }
}

CTL_EGRESS {
    Counter<int, int>(  1, PSA_CounterType_t.PACKETS)             c_p;
    Counter<int, int>( 10, PSA_CounterType_t.BYTES)               c_b;
    Counter<int, int>(100, PSA_CounterType_t.PACKETS_AND_BYTES)   c_pb;

    DirectCounter<int>(PSA_CounterType_t.PACKETS)           dc_p;
    DirectCounter<int>(PSA_CounterType_t.BYTES)             dc_b;
    DirectCounter<int>(PSA_CounterType_t.PACKETS_AND_BYTES) dc_pb;

    Meter<int>(1,   PSA_MeterType_t.BYTES)   m_b;
    Meter<int>(256, PSA_MeterType_t.PACKETS) m_p;

    DirectMeter(PSA_MeterType_t.BYTES)   dm_b;
    DirectMeter(PSA_MeterType_t.PACKETS) dm_p;

    Register<bit<16>, int>(32w65536) reg16;


    action forward() {}
    action _nop() {}

    action bcast() {
        c_p.count(1);

        c_b.count(2);
        c_b.count(2);

        c_pb.count(3);
        c_pb.count(3);
        c_pb.count(3);

        m_b.execute(1, PSA_MeterColor_t.GREEN);
        m_p.execute(11, PSA_MeterColor_t.GREEN);


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

CTL_EMIT {
    apply {
        buffer.emit(hdr.dstAddr);
        buffer.emit(hdr.srcAddr);
        buffer.emit(hdr.etherType);
    }
}

#include "psa-boilerplate-post.p4"
