
#include "common-boilerplate-pre.p4"

struct metadata {
}

header dummy_t {
    bool b1;
    bool b2;
    bool b3;
    bool b4;
    bit<4> padding;
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
    bool tmp_bool = true;

    apply {
        SET_EGRESS_PORT(GET_INGRESS_PORT());
        hdr.dummy.b1 = hdr.dummy.b1 || tmp_bool;
        hdr.dummy.b2 = !(hdr.dummy.b2 && tmp_bool) == tmp_bool;
        hdr.dummy.b3 = (hdr.dummy.b3 || tmp_bool) != false;
        hdr.dummy.b4 = hdr.dummy.b4 ? false : true;
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.dummy);
    }
}

#include "common-boilerplate-post.p4"
