// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Eotvos Lorand University, Budapest, Hungary

#include "common-boilerplate-pre.p4"

struct metadata {
}

struct headers {
    ethernet_t  ethernet;
}

PARSER {
    state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

CTL_MAIN {
    action set_ethertype() {
        hdr.ethernet.etherType = 0x1234;
        hdr.ethernet.dstAddr   = 0b0001_0010_0011_0100_0101_0110_0111_1000_1001_1010_1011_1100;

        standard_metadata.egress_port = 0;
    }

    table dmac {
        actions = {
            set_ethertype;
        }

        key = {
        }

        size = 1;

        default_action = set_ethertype;
    }

    apply {
        dmac.apply();
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.ethernet);
    }
}

#include "common-boilerplate-post.p4"
