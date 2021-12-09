#include "common-boilerplate-pre.p4"

struct metadata {
}

struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
}

PARSER {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition accept;
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}

CTL_MAIN {
    DECLARE_DIGEST(mac_learn_digest_t, mac_learn_digest)

    @name(".forward") action forward(PortId_t port) {
        SET_EGRESS_PORT(port);
    }
    @name(".bcast") action bcast() {
        SET_EGRESS_PORT(PortId_const(100));
    }
    @name(".mac_learn") action mac_learn() {
        CALL_DIGEST(mac_learn_digest_t, mac_learn_digest, 1024, ({ hdr.ethernet.srcAddr, GET_INGRESS_PORT() }));
    }
    @name("._nop") action _nop() {
    }
    @name("._nop") action testing(bit<32> arg1, bit<32> arg2) {
    }
    @name(".dmac") table dmac {
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
    @name(".smac") table smac {
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
    apply {}
}

#include "common-boilerplate-post.p4"
