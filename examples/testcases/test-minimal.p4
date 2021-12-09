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

register<bit<32>>(1) test_register;

CTL_MAIN {
    action _drop() {
        mark_to_drop(standard_metadata);
    }

    action forward(bit<9> port) {
        standard_metadata.egress_port = port;
    }

    action mac_learn() {
    }

    action _nop() {
    }

    action bcast() {
		standard_metadata.egress_port = 100;
    }

    table smac {
        key = {
            standard_metadata.ingress_port: exact;
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
            standard_metadata.ingress_port: exact;
        }
        actions = {
            bcast;
            forward;
        }
        default_action = bcast;
    }

    apply {
        bit<32> test_val;
        test_register.read(test_val, 0);
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
