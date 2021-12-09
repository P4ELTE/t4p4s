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
       if (hdr.dummy.f1 == (bit<2>)0) {
            if (hdr.dummy.f2 == (bit<2>)1) {
                hdr.dummy.f1 = hdr.dummy.f1 + (bit<2>)3;
                hdr.dummy.f2 = hdr.dummy.f2 + (bit<2>)2;
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
