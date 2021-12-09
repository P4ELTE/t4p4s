#include "common-boilerplate-pre.p4"

header dummy_t {
    int<2> f1;
    int<2> f2;
    int<2> f3;
    int<2> f4;
    int<2> f5;
    int<2> f6;
    int<2> f7;
    int<2> f8;
    int<2> f9;
    int<3> f10;
    int<3> padding;
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
    const int tmp_int = 1;
    int<(2*tmp_int)> tmp_int2 = -2s0;

    apply {
       hdr.dummy.f1 = hdr.dummy.f1 + 2s1;
       hdr.dummy.f2 = tmp_int2 - hdr.dummy.f2;
       hdr.dummy.f3 = hdr.dummy.f3 * tmp_int2 + tmp_int2;
       hdr.dummy.f4 = hdr.dummy.f4 | tmp_int2;
       hdr.dummy.f5 = ~(hdr.dummy.f5 & tmp_int2);
       hdr.dummy.f6 = hdr.dummy.f6 ^ tmp_int2;
       hdr.dummy.f7 = (hdr.dummy.f7 - tmp_int) |-| tmp_int;
       hdr.dummy.f8 = (hdr.dummy.f8 + tmp_int) |+| tmp_int;
       hdr.dummy.f9 = (int<2>)((hdr.dummy.f9 - 1)[1:0]);
       hdr.dummy.f10[0:0] = (((hdr.dummy.f10 >> 123) == (int<3>)0)?1w1:1w0);
       hdr.dummy.f10[2:1] = (((hdr.dummy.f10 ++ (int<3>)-1) >> 1) << 2)[3:2];
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.dummy);
    }
}

#include "common-boilerplate-post.p4"
