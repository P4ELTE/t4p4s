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
    apply {
        SET_EGRESS_PORT(GET_INGRESS_PORT());
        if (hdr.dummy.f1 == 2w0) {
            if (hdr.dummy.f2 == 2w1) {
                hdr.dummy.f1 = hdr.dummy.f1 + 2w3;
                hdr.dummy.f2 = hdr.dummy.f2 + 2w2;
            } else {
                exit;
            }
       }
    }
}


CTL_EMIT {
    apply {
        packet.emit(hdr.dummy);
    }
}

#include "common-boilerplate-post.p4"
