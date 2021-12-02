#include "psa-boilerplate-pre.p4"

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

#undef USE_DEFAULT_CTL_INGRESS
#define USE_DEFAULT_CTL_EGRESS

struct digest_dummy_t {
    bit<8> data;
}

control DigestControl() {
    Digest<digest_dummy_t>() digest_var;

    apply {
        digest_var.pack({123});
    }
}

control Chained() {
    apply {
        DigestControl.apply();
    }
}

CTL_INGRESS {
    apply {
        DigestControl.apply();
        Chained.apply();
        ostd.egress_port = (PortId_t)12345;
    }
}

CTL_EMIT {
    apply {
        buffer.emit(hdr.dummy);
    }
}

#include "psa-boilerplate-post.p4"
