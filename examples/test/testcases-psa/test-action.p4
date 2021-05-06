
#include "psa-boilerplate-pre.p4"

struct metadata {
}

header dummy_t {
    bool   f1;
    bool   f2;
    bool   f3;
    bool   f4;
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
    action action_one(out dummy_t out_d, bool f1, bool f2, bool f3, bool f4) {
        out_d.f1 = f1;
        out_d.f2 = f2;
        out_d.f3 = f3;
        out_d.f4 = f4;
    }

    apply {
        action_one(hdr.dummy, hdr.dummy.f1 || true, hdr.dummy.f2 || true, hdr.dummy.f3 || true, hdr.dummy.f4 || true);
    }
}

CTL_EMIT {
    apply {
        buffer.emit(hdr.dummy);
    }
}

#include "psa-boilerplate-post.p4"
