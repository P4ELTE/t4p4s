#include "common-boilerplate-pre.p4"

header dummy_t {
    bit<8> ignored;
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
    apply {
        // intentionally called twice
    	MARK_TO_DROP();
    	MARK_TO_DROP();
        SET_EGRESS_PORT((PortId_t)23);
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.dummy);
    }
}

#include "common-boilerplate-post.p4"
