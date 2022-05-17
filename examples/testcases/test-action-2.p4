#include "common-boilerplate-pre.p4"

header dummy_t {
    bit<2> f1;
    bit<2> f2;
    bit<4> padding;
}

struct metadata {
}

struct headers {
    dummy_t dummy;
}

PARSER {
    state start {
        packet.extract(hdr.dummy);
        transition accept;
    }
}

CTL_MAIN {
    bit<2> z = 0;
    action action1(in bit<2> f1, bit<2> f2) {
        hdr.dummy.f1 = f1;
        hdr.dummy.f2 = f2;
        z = f1;
    }
    action action2(in bit<2> f1, bit<2> f2) {
        hdr.dummy.f1 = f1;
        hdr.dummy.f2 = f2;
    }
    table t {
        actions = {
            action1(2w1);
            action2(z);
        }
        const default_action = action2(z,2w1);
        key = {
            hdr.dummy.f1: exact;
        }
        size = 512;
        const entries = {
            (0x00) : action1(2w1, 2w1);
            (0x01) : action2(z, 2w2);
        }
    }
    apply {
        SET_EGRESS_PORT(GET_INGRESS_PORT());
        t.apply();
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.dummy);
    }
}

#include "common-boilerplate-post.p4"
