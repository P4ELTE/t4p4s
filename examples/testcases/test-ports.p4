
#include "common-boilerplate-pre.p4"

struct metadata {
}

struct headers {
    bits32_t ingress_port;
    bits32_t egress_port;
}

PARSER {
    state start {
        packet.extract(hdr.ingress_port);
        packet.extract(hdr.egress_port);
        transition accept;
    }
}

CTL_MAIN {
    apply {
        SET_EGRESS_PORT(GET_INGRESS_PORT());

        hdr.ingress_port.f32 = (bit<32>)GET_INGRESS_PORT();
        hdr.egress_port.f32  = (bit<32>)GET_EGRESS_PORT();
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.ingress_port);
        packet.emit(hdr.egress_port);
    }
}

#include "common-boilerplate-post.p4"
