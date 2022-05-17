#include "common-boilerplate-pre.p4"

header dummy_t {
    bit<1> f1;
    bit<1> f2;
    bit<1> f3;
    bit<1> f4;
    bit<1> f5;
    bit<1> f6;
    bit<1> f7;
    bit<2> f8;
    bit<2> f9;
    bit<2> f10;
    bit<1> f13;
    bit<2> padding;
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
    bit tmp_bit = 1;
    bit<2> tmp_bit2 = 2w3;
    
    apply {
        SET_EGRESS_PORT(GET_INGRESS_PORT());
        hdr.dummy.f1 = hdr.dummy.f1 + (bit<1>)true;
        hdr.dummy.f2 = tmp_bit - hdr.dummy.f2;
        hdr.dummy.f3 = hdr.dummy.f3 * tmp_bit + tmp_bit;
        hdr.dummy.f4 = (bit<1>)(hdr.dummy.f4 == 0);
        hdr.dummy.f5 = (bit<1>)(hdr.dummy.f5 != 1);
        hdr.dummy.f6 = (bit<1>)(hdr.dummy.f6 > 0 || hdr.dummy.f6 >= 0);
        hdr.dummy.f7 = (bit<1>)(hdr.dummy.f7 < 1 || hdr.dummy.f7 <= 1);
        hdr.dummy.f8 = hdr.dummy.f8 | tmp_bit2;
        hdr.dummy.f9 = ~(hdr.dummy.f9 & tmp_bit2);
        hdr.dummy.f10 = hdr.dummy.f10 ^ tmp_bit2;
        hdr.dummy.f13 = (hdr.dummy.f13 ++ (hdr.dummy.f13 + 1))[0:0];
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.dummy);
    }
}

#include "common-boilerplate-post.p4"
