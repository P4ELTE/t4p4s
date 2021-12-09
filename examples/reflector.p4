/* -*- P4_16 -*- */
#include "common-boilerplate-pre.p4"

struct metadata {
    /* empty */
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
    action swap_mac_addresses() {
        macAddr_t tmp_mac;
        tmp_mac = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = hdr.ethernet.srcAddr;
        hdr.ethernet.srcAddr = tmp_mac;

        //send it back to the same port
        SET_EGRESS_PORT(GET_INGRESS_PORT());
    }

    apply {
       swap_mac_addresses();
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.ethernet);
    }
}

#include "common-boilerplate-post.p4"
