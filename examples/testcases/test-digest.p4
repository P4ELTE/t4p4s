#include "common-boilerplate-pre.p4"

header dummy_t {
    bit<8> addr;
}

struct metadata {
}

struct headers {
    dummy_t dummy;
}

struct learn_digest_t {
    bit<8> addr;
    bit<8> dd;
}

PARSER {
    state start {
        packet.extract(hdr.dummy);
        transition accept;
    }
}

CTL_MAIN {
    DECLARE_DIGEST(learn_digest_t, learn_digest)

    action found(bit<8> dd) {
        hdr.dummy.addr = dd;
    }
    action learn() {
        CALL_DIGEST(learn_digest_t, learn_digest, 1024, ({ hdr.dummy.addr, 8w2 }));
    }
    table t {
        actions = {
            found;
            learn;
        }
        key = {
            hdr.dummy.addr: exact;
        }
        size = 512;
    }
    apply {
        SET_EGRESS_PORT(GET_INGRESS_PORT());
        t.apply();
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.dummy);
    }
}

#include "common-boilerplate-post.p4"
