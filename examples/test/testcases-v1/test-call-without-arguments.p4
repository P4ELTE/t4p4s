// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "v1-boilerplate-pre.p4"

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

extern void dummy_crypto();

CTL_INGRESS {
    apply {
        log_msg("Before    = {}",{8w0});
        dummy_crypto();
        log_msg("After    = {}",{8w0});
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.ethernet);
    }
}

#include "v1-boilerplate-post.p4"
