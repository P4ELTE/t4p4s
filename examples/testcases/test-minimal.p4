/* -*- P4_16 -*- */

#include "common-boilerplate-pre.p4"

struct headers {
    ethernet_t ethernet;
}

struct metadata {
}

PARSER {
    state start {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0x0800: accept;
            default: accept;
        }
    }
}

DECLARE_REGISTER(bit<32>, 1, test_register)

CTL_MAIN {
    action _drop() {
        MARK_TO_DROP();
    }

    action forward(PortId_t port) {
        SET_EGRESS_PORT(port);
    }

    action mac_learn() {
    }

    action _nop() {
    }

    action bcast() {
        SET_EGRESS_PORT(PortId_const(100));
    }

    table smac {
        key = {
            GET_INGRESS_PORT(): exact;
        }
        actions = {
            forward;
            mac_learn;
            _drop;
            _nop;
        }
        default_action = _drop;
    }

    table dmac {
        key = {
            GET_INGRESS_PORT(): exact;
        }
        actions = {
            bcast;
            forward;
        }
        default_action = bcast;
    }

    apply {
        bit<32> test_val;
        REGISTER_READ(test_val, test_register, 0);
        smac.apply();
        dmac.apply();
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.ethernet);
    }
}

#include "common-boilerplate-post.p4"
