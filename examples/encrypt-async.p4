// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

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

extern void async_encrypt<T>(in T offset);

CTL_MAIN {
    apply {
        async_encrypt({8w34});
        SET_EGRESS_PORT(1);
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.ethernet);
    }
}

#include "common-boilerplate-post.p4"

