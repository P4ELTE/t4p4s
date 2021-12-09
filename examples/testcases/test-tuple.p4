#include "common-boilerplate-pre.p4"

header dummy_t {
    bit<1> f1;
    bit<2> f2;
    bit<5> padding;
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
    tuple<> empty_tuple = {};
    tuple<bit<1>, bit<2>, bit<5>> x = { hdr.dummy.f1, hdr.dummy.f2, hdr.dummy.padding };
    
    apply {
       hdr.dummy = {1w1, 2w3, 5w0};
    }
}


CTL_EMIT {
    apply {
        packet.emit(hdr.dummy);
    }
}

#include "common-boilerplate-post.p4"
