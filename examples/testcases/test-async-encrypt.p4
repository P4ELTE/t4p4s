// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "common-boilerplate-pre.p4"

struct metadata {
}

struct headers {
    bits8_t offset;
}

PARSER {
    state start {
        packet.extract(hdr.offset);
        transition accept;
    }
}

extern void async_encrypt<T>(in T offset);

CTL_MAIN {
    apply {
        async_encrypt({hdr.offset.f8 + 8w1});
        SET_EGRESS_PORT(GET_INGRESS_PORT());
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.offset);
    }
}

#include "common-boilerplate-post.p4"
