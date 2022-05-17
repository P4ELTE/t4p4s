// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "common-boilerplate-pre.p4"

struct metadata {
}

struct headers {
    ethernet_t ethernet;
    bits8_t    h8;
}

PARSER {
    state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

CTL_MAIN {
    action setValid_h8() {
        hdr.h8.setValid();
        hdr.h8.f8 = 0xEB;
    }

    table dmac {
        actions = {
            setValid_h8;
        }

        key = {
        }

        size = 1;

        default_action = setValid_h8;
    }

    apply {
        SET_EGRESS_PORT(GET_INGRESS_PORT());
        dmac.apply();
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.h8);
    }
}

#include "common-boilerplate-post.p4"
