#include "psa-boilerplate-pre.p4"

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

#undef USE_DEFAULT_CTL_INGRESS
#define USE_DEFAULT_CTL_EGRESS

CTL_INGRESS {
    Digest<learn_digest_t>() learn_digest;
    action found(bit<8> dd) {
        hdr.dummy.addr = dd;
    }
    action learn() {
    	learn_digest.pack({ hdr.dummy.addr, (bit<8>)2 });
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
        t.apply();
        ostd.egress_port = (PortId_t)12345;
    }
}

CTL_EMIT {
    apply {
        buffer.emit(hdr.dummy);
    }
}

#include "psa-boilerplate-post.p4"
