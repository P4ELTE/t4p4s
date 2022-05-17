#include "common-boilerplate-pre.p4"

type bit<1> one_bit_t;

header dummy_t {
    one_bit_t f1;
    bit<7> padding;
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
    one_bit_t one = 1;
    
    apply {
        SET_EGRESS_PORT(GET_INGRESS_PORT());
        hdr.dummy.f1 = one;
    }
}


CTL_EMIT {
    apply {
        packet.emit(hdr.dummy);
    }
}

#include "common-boilerplate-post.p4"
