#include "common-boilerplate-pre.p4"

struct metadata {
}

header dummy_t {
    bit<8> addr;
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

struct digest_dummy_t {
    bit<8> data;
}

control DigestControl() {
    DECLARE_DIGEST(digest_dummy_t, digest_var)

    apply {
        CALL_DIGEST(digest_dummy_t, digest_var, 1024, ({123}));
    }
}

control Chained() {
    apply {
        DigestControl.apply();
    }
}

CTL_MAIN {
    apply {
        DigestControl.apply();
        Chained.apply();
        SET_EGRESS_PORT(GET_INGRESS_PORT());
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.dummy);
    }
}

#include "common-boilerplate-post.p4"
