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
extern void dummy_crypto<T1>(in T1 data1);
extern void dummy_crypto<T1,T2>(in T1 data1, in T2 data2);

CTL_INGRESS {
    apply {
        dummy_crypto();
        dummy_crypto({8w1});
        dummy_crypto({-8w1});
        dummy_crypto({8w1}, {8w1});
        dummy_crypto({-8w1}, {-8w1});
        dummy_crypto({16w1}, {32w1});
        dummy_crypto({-16w1}, {-32w1});
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.ethernet);
    }
}

#include "v1-boilerplate-post.p4"
