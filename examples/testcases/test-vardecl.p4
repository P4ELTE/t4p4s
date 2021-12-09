
#include "common-boilerplate-pre.p4"

header dummy_t {
    bit<1> f1;
    bit<1> f2;
    bit<1> f3;
    bit<1> f4;
    bit<1> f5;
    bit<3> padding;
}

struct metadata {
}

struct headers {
    dummy_t dummy;
}

PARSER {
    bit<1> tmp = 1w1;

    state start {
        bit<1> tmp2 = 1w1;

        packet.extract(hdr.dummy);
        hdr.dummy.f1 = hdr.dummy.f1 + tmp;
        hdr.dummy.f2 = hdr.dummy.f2 + tmp2;

        transition accept;
    }
}

CTL_MAIN {
    bit<1> tmp = 1w1;

    action dummy_action() {
       bit<1> tmp2 = 1w1;
       hdr.dummy.f5 = hdr.dummy.f5 + tmp2;
    }
    
    apply {
       hdr.dummy.f3 = hdr.dummy.f3 + tmp;
       bit<1> tmp2 = 1w1;
       hdr.dummy.f4 = hdr.dummy.f4 + tmp2;
       dummy_action();
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.dummy);
    }
}

#include "common-boilerplate-post.p4"
