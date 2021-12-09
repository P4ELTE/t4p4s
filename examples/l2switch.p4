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
    DECLARE_DIGEST(mac_learn_digest_t, mac_learn_digest)

    action forward(PortId_t port) {
        SET_EGRESS_PORT(port);
    }

    action bcast() {
        SET_EGRESS_PORT(PortId_const(100)); // Broadcast port
    }

    action mac_learn() {
        CALL_DIGEST(mac_learn_digest_t, mac_learn_digest, 1024, ({ hdr.ethernet.srcAddr, GET_INGRESS_PORT() }));
    }

    action drop() {
        MARK_TO_DROP();
        exit;
    }

    action _nop() {
    }

    table dmac {
        actions = {
            forward;
            bcast;
            drop;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        default_action = bcast();
        size = 512;
    }

    table smac {
        actions = {
            mac_learn;
            _nop;
            drop;
        }
        key = {
            hdr.ethernet.srcAddr: exact;
        }
        default_action = mac_learn();
        size = 512;
    }

    apply {
        smac.apply();
        dmac.apply();
    }

}

CTL_EMIT {
    apply {}
}

#include "common-boilerplate-post.p4"
