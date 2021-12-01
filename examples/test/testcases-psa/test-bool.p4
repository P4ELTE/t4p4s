
#include "psa-boilerplate-pre.p4"

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

CTL_EGRESS {
    bool tmp_bool = true;

    apply {
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
