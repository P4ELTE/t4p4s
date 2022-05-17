
#include "common-boilerplate-pre.p4"

struct metadata {
}

header dummy_t {
	bit<32> addr;
}

struct headers {
    ethernet_t ethernet;
    bits8_t    result1;
    bits8_t    result2;
}

PARSER {
    state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

CTL_MAIN {
    action result1(bit<8> result) { hdr.result1.f8 = result; }
    action result1_miss()         { hdr.result1.f8 = 0xAA; }
    action result2(bit<8> result) { hdr.result2.f8 = result; }
    action result2_miss()         { hdr.result2.f8 = 0xAA; }

    table t1 {
        actions = {
            result1;
            result1_miss;
        }

        key = {
            hdr.ethernet.srcAddr: exact;
        }

        size = 16;

        default_action = result1_miss;

        const entries = {
            0x0012_3456_7890 : result1(0x01);
            0x9078_5634_1200 : result1(0xFF);
            _                : result1_miss;
        }
    }

    table t2 {
        actions = {
            result2;
            result2_miss;
        }

        key = {
            hdr.ethernet.dstAddr: exact;
            hdr.ethernet.srcAddr: exact;
        }

        size = 16;

        default_action = result2_miss;

        const entries = {
            (0x0000_1234_5678, 0x0000_9ABC_DEF0) : result2(0x01);
            (0x0000_0_D15EA5E, 0X0000_DEAD_BEEF) : result2(0x02);
            (0x0000_DEAD_10CC, 0x0000_BAAA_AAAD) : result2(0x03);
            (100, 200)                           : result2(0x04);
            (300, 400)                           : result2(0x05);
            (_, _)                               : result2_miss;
        }
    }

    apply {
        SET_EGRESS_PORT(GET_INGRESS_PORT());

        hdr.result1.setValid();
        hdr.result2.setValid();

        t1.apply();
        t2.apply();

        hdr.ethernet.setInvalid();
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.result1);
        packet.emit(hdr.result2);
    }
}

#include "common-boilerplate-post.p4"
