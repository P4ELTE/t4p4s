#include "common-boilerplate-pre.p4"

const bit<16> IP = 0x0800;

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

    register<bit<16>>(1) reg1;
    bit<16> var1;

    action _nop() {
    }
    table dmac {
        actions = {
            forward;
            bcast;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        size = 512;
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
        @atomic {
            reg1.read(var1,0);
            var1 = var1 + 1;
            reg1.write(0, var1);
        }

    }
}

CTL_EMIT {
    apply {}
}

#include "common-boilerplate-post.p4"
