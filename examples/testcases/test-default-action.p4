#include "common-boilerplate-pre.p4"

header dummy_t {
    bit<8> f;
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
    action def_act() {
        hdr.dummy.f = 8w1;
    }
    table t {
        actions = {
            def_act;
        }
        key = {
            hdr.dummy.f: exact;
        }
        size = 512;
        default_action = def_act;
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
