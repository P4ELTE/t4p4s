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
    action action_one(in bit<2> f1, bit<2> f2) {
        hdr.dummy.f1 = f1;
        hdr.dummy.f2 = f2;
        z = f1;
    }
    action action_two(in bit<2> f1, bit<2> f2) {
        hdr.dummy.f1 = f1;
        hdr.dummy.f2 = f2;
    }
    table t {
	actions = {
	    action_one((bit<2>)1);
            action_two(z);
	}
        const default_action = action_two(z,(bit<2>)1);
        key = {
            hdr.dummy.f1: exact;
        }
        size = 512;
        const entries = {
            (0x00) : action_one((bit<2>)1, (bit<2>)1);
            (0x01) : action_two(z, (bit<2>)2);
        }
    }
    apply {
        t.apply();
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.dummy);
    }
}

#include "common-boilerplate-post.p4"
