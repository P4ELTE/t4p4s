
#include "psa-boilerplate-pre.p4"

struct metadata {
}

header dummy_t {
    bool b1;
    bool b2;
    bool b3;
    bool b4;
    bit<4> padding;

    // padded_bool_t f1;
    // padded_bool_t f2;
    // padded_bool_t f3;
    // padded_bool_t f4;
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

CTL_EGRESS {
    bool tmp_bool = true;

    apply {
       // hdr.dummy.f1.b = hdr.dummy.f1.b || tmp_bool;
       // hdr.dummy.f2.b = !(hdr.dummy.f2.b && tmp_bool) == tmp_bool;
       // hdr.dummy.f3.b = (hdr.dummy.f3.b || tmp_bool) != false;
       // hdr.dummy.f4.b = hdr.dummy.f4.b ? false : true;

       hdr.dummy.b1 = hdr.dummy.b1 || tmp_bool;
       hdr.dummy.b2 = !(hdr.dummy.b2 && tmp_bool) == tmp_bool;
       hdr.dummy.b3 = (hdr.dummy.b3 || tmp_bool) != false;
       hdr.dummy.b4 = hdr.dummy.b4 ? false : true;
    }
}

CTL_EMIT {
    apply {
        buffer.emit(hdr.dummy);
    }
}

#include "psa-boilerplate-post.p4"
