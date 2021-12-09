#include "common-boilerplate-pre.p4"

struct metadata {
}

struct headers {
    ethernet_t ethernet;
}

PARSER {
    state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

CTL_MAIN {
    action forward(PortId_t port) {
        SET_EGRESS_PORT(port);
    }

    action forward_rewrite(PortId_t port, macAddr_t mac) {
        SET_EGRESS_PORT(port);
        hdr.ethernet.srcAddr = mac;
    }

    action _drop() {
        MARK_TO_DROP();
    }

    table t_fwd {
        actions = {
            forward;
            forward_rewrite;
            _drop;
        }
        key = {
            GET_INGRESS_PORT(): exact;
        }
        size = 2048;
    }

    apply {
        t_fwd.apply();
    }
}

CTL_EMIT {
    apply {}
}

#include "common-boilerplate-post.p4"
