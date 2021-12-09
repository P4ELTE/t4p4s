#include "common-boilerplate-pre.p4"

header dummy_t {
    bit<1> f7;
    bit<1> f6;
    bit<1> f5;
    bit<1> f4;
    bit<1> f3;
    bit<1> f2;
    bit<1> f1;
    bit<1> f0;
}

struct metadata {
}

struct headers {
    dummy_t dummy0;
    dummy_t dummy1;
    dummy_t dummy2;
    dummy_t dummy3;
    dummy_t dummy4;
    dummy_t dummy5;
    dummy_t dummy6;
    dummy_t dummy7;
}

PARSER {
    state start {
        packet.extract(hdr.dummy0);
        packet.extract(hdr.dummy1);
        packet.extract(hdr.dummy2);
        packet.extract(hdr.dummy3);
        packet.extract(hdr.dummy4);
        packet.extract(hdr.dummy5);
        packet.extract(hdr.dummy6);
        packet.extract(hdr.dummy7);
        transition accept;
    }
}

CTL_MAIN {
    bit<1> one = 1;

    apply {
       hdr.dummy0.f0  = hdr.dummy0.f0 + one + 1w0;

       // tests if modifying non-byte-aligned fields works properly
       hdr.dummy1.f1 = hdr.dummy1.f1 + one + 1w0;
       hdr.dummy2.f2 = hdr.dummy2.f2 + one + 1w0;
       hdr.dummy3.f3 = hdr.dummy3.f3 + one + 1w0;
       hdr.dummy4.f4 = hdr.dummy4.f4 + one + 1w0;
       hdr.dummy5.f5 = hdr.dummy5.f5 + one + 1w0;
       hdr.dummy6.f6 = hdr.dummy6.f6 + one + 1w0;
       hdr.dummy7.f7 = hdr.dummy7.f7 + one + 1w0;
    }
}


CTL_EMIT {
    apply {
        packet.emit(hdr.dummy0);
        packet.emit(hdr.dummy1);
        packet.emit(hdr.dummy2);
        packet.emit(hdr.dummy3);
        packet.emit(hdr.dummy4);
        packet.emit(hdr.dummy5);
        packet.emit(hdr.dummy6);
        packet.emit(hdr.dummy7);
    }
}

#include "common-boilerplate-post.p4"
