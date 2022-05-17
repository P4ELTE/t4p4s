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
        SET_EGRESS_PORT(PortId_const(100));
    }
    action mac_learn() {
        CALL_DIGEST(mac_learn_digest_t, mac_learn_digest, 1024, ({ hdr.ethernet.srcAddr, GET_INGRESS_PORT() }));
    }
    action _nop() {
        SET_EGRESS_PORT(GET_EGRESS_PORT());
    }
    action testing(bit<32> arg1, bit<32> arg2) {
        SET_EGRESS_PORT(GET_EGRESS_PORT());
    }
    table dmac {
        actions = {
            forward;
            bcast;
            testing;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        size = 512;

        #ifdef T4P4S_TEST_1
            const entries = {
                (0xD15EA5E): bcast();
            }
        #elif T4P4S_TEST_abc
            const entries = {
                (0xDEAD_10CC): testing(510, 0xffff_ffff);
            }
        #endif
    }
    table smac {
        actions = {
            mac_learn;
            _nop;
        }
        key = {
            hdr.ethernet.srcAddr: exact;
        }
        size = 512;
    }
    apply {
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
