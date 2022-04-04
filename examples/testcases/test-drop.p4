#include "common-boilerplate-pre.p4"

header dummy_t {
    bool f1;
    bool f2;
    bool f3;
    bool f4;
    bit<4> padding;
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
    	MARK_TO_DROP();
    	standard_metadata.egress_port = (PortId_t)23;
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.dummy);
    }
}

#include "common-boilerplate-post.p4"
