// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "common-boilerplate-pre.p4"

struct metadata {
}

struct headers {
    ethernet_t ethernet;
    hdr1_t     h1;
}

PARSER {
    state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

CTL_MAIN {
    action setValid_h1() {
        hdr.h1.setValid();
        hdr.h1.byte1 = 0xEB;
    }

    table dmac {
        actions = {
            setValid_h1;
        }

        key = {
        }

        size = 1;

        default_action = setValid_h1;
    }

    apply {
        dmac.apply();
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.h1);
    }
}

#include "common-boilerplate-post.p4"
