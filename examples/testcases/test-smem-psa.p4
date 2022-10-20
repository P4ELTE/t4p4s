// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "common-boilerplate-pre.p4"

DECLARE_REGISTER(bit<32>, 32w100, debug)
DECLARE_REGISTER(bit<1>, 32w1, reg1)


struct gtp_metadata_t {
    bit<32> data32;
    bit<32> regtmp32;

    bit<16> data16;
    bit<16> regtmp16;

    bit<1>  data1;
    bit<1>  regtmp1;
    bit<6>  padding;
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

CTL_MAIN {
    Counter<int, int>(  1, PSA_CounterType_t.PACKETS)             c_p;
    Counter<int, int>( 10, PSA_CounterType_t.BYTES)               c_b;
    Counter<int, int>(100, PSA_CounterType_t.PACKETS_AND_BYTES)   c_pb;

    DirectCounter<int>(PSA_CounterType_t.PACKETS)           dc_p;
    DirectCounter<int>(PSA_CounterType_t.BYTES)             dc_b;
    DirectCounter<int>(PSA_CounterType_t.PACKETS_AND_BYTES) dc_pb;

    DECLARE_METER(1,   int, bytes, BYTES, m_b);
    DECLARE_METER(256, int, packets, PACKETS, m_p);

    DirectMeter(PSA_MeterType_t.BYTES)   dm_b;
    DirectMeter(PSA_MeterType_t.PACKETS) dm_p;

    DECLARE_REGISTER(bit<16>, 32w65536, reg16)


    action forward() {}
    action _nop() {}

    action bcast() {
        c_p.count(1);

        c_b.count(2);
        c_b.count(2);

        c_pb.count(3);
        c_pb.count(3);
        c_pb.count(3);

        MeterColor_t(bit<32>) out1;
        MeterColor_t(bit<32>) out2;
        METER_EXECUTE_COLOR(out1, m_b, 1,  METER_GREEN);
        METER_EXECUTE_COLOR(out2, m_p, 11, METER_GREEN);


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
        SET_EGRESS_PORT(GET_INGRESS_PORT());
        dmac.apply();
        smac.apply();
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.dstAddr);
        packet.emit(hdr.srcAddr);
        packet.emit(hdr.etherType);
    }
}

#include "common-boilerplate-post.p4"
